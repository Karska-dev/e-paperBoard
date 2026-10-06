#!/usr/bin/env python3
"""Test server: a stand-in for the real one, standard library only.

Speaks docs/SERVER_CONTRACT.md and nothing more. Run it on a computer in the
board's Wi-Fi network; it prints the line to put into secrets.h:

    python3 tools/test_server.py        (Ctrl+C stops it)

TECHNIQUE: a test image where each mark answers one question.

    +----------------------------+   frame      all 800 x 480 pixels arrive
    | #                          |   # corner   orientation (must be top-left)
    |                            |   row of N   which screen (home = 1, next = 2...)
    |   [] [] []                 |   white      bit 1 = white is not swapped
    +----------------------------+
"""

import argparse
import socket
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

WIDTH, HEIGHT = 800, 480
ROW_BYTES = WIDTH // 8

# Same ids, same order, as kScreenIds in firmware/src/pure/screens.h.
SCREEN_IDS = ["home", "time-left", "year-dots", "night-sky", "family-week", "weather", "word"]

# Sleep time sent to the device. Short, so a timer wake (and its 304) can be
# watched without waiting.
NEXT_WAKE_SECONDS = 120


def fill_rect(frame, left, top, width, height):
    """Paints a black rectangle.

    TECHNIQUE: bit mask. Pixel (x, y) is bit (7 - x % 8) of byte
    (y * 100 + x // 8). Bit 1 is white, so black means CLEARING the bit:
    AND with a mask that is 0 only there.
    """
    for y in range(top, top + height):
        for x in range(left, left + width):
            frame[y * ROW_BYTES + x // 8] &= ~(0x80 >> (x % 8)) & 0xFF


def make_frame(screen_index):
    frame = bytearray(b"\xff" * (ROW_BYTES * HEIGHT))  # every bit 1: all white

    border = 8
    fill_rect(frame, 0, 0, WIDTH, border)
    fill_rect(frame, 0, HEIGHT - border, WIDTH, border)
    fill_rect(frame, 0, 0, border, HEIGHT)
    fill_rect(frame, WIDTH - border, 0, border, HEIGHT)

    fill_rect(frame, 30, 30, 50, 50)  # orientation mark, top-left

    square, gap = 80, 20
    for i in range(screen_index + 1):
        fill_rect(frame, 60 + i * (square + gap), 200, square, square)

    return bytes(frame)


# Built once. An ETag changes only when its image does; here never, so every
# repeated request gets a 304.
FRAMES = {name: make_frame(index) for index, name in enumerate(SCREEN_IDS)}
ETAGS = {name: '"%s-test-1"' % name for name in SCREEN_IDS}


class Handler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        """Silences the built-in access log; do_GET prints clearer lines."""

    def do_GET(self):
        url = urlparse(self.path)
        query = parse_qs(url.query)
        screen = query.get("id", [""])[0]

        if url.path != "/screen" or screen not in FRAMES:
            self.send_error(404, "unknown path or screen id")
            return

        # What the device told us about itself.
        print(
            "  screen=%s wake=%s battery=%s mV (%s%%) low=%s firmware=%s"
            % (
                screen,
                query.get("wake", ["?"])[0],
                query.get("bat_mv", ["?"])[0],
                query.get("bat_pct", ["?"])[0],
                query.get("low", ["?"])[0],
                query.get("fw", ["?"])[0],
            )
        )

        etag = ETAGS[screen]
        unchanged = self.headers.get("If-None-Match") == etag

        # send_response() also adds the Date header: the device sets its
        # clock from it.
        self.send_response(304 if unchanged else 200)
        self.send_header("ETag", etag)
        self.send_header("X-Next-Wake", str(NEXT_WAKE_SECONDS))
        if unchanged:
            self.end_headers()
            print("  -> 304 Not Modified (device keeps its image)")
            return

        body = FRAMES[screen]
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
        print("  -> 200 OK, %d bytes" % len(body))


def local_address():
    """This computer's address in the local network.

    TRICK: "connecting" a UDP socket sends nothing, but makes the system
    pick the interface it would use; its address is then read back.
    """
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        probe.connect(("192.0.2.1", 9))  # A reserved example address; never contacted.
        return probe.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        probe.close()


def main():
    parser = argparse.ArgumentParser(description="Test server for the e-paperBoard firmware.")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()

    # "0.0.0.0": accept connections on every interface, not only from this
    # computer.
    server = HTTPServer(("0.0.0.0", args.port), Handler)
    print("Test server running. Put this into firmware/include/secrets.h:")
    print('    #define EPB_SERVER_URL "http://%s:%d"' % (local_address(), args.port))
    print("Waiting for the board (Ctrl+C to stop)...")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
