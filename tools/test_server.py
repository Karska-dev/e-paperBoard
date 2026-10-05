#!/usr/bin/env python3
"""A tiny stand-in for the real server, for testing the firmware.

It speaks the contract in docs/SERVER_CONTRACT.md and nothing more: for each
screen id it returns a 48,000-byte test image with an ETag, and it answers
304 Not Modified when the device already has that image.

Run it on a computer in the same Wi-Fi network as the board:

    python3 tools/test_server.py

It prints the address to put into firmware/include/secrets.h. It uses only
Python's standard library, so there is nothing to install. Stop it with
Ctrl+C.

WHAT THE TEST IMAGE LOOKS LIKE (and why):
  - a black frame around the edge: shows that all 800 x 480 pixels arrive
    and that nothing is cut off or shifted
  - a black square in the TOP-LEFT corner: shows the orientation. If it
    appears in another corner, the image is mirrored or rotated
  - a row of black squares in the middle, one per screen: home has 1, the
    next screen 2, and so on. Flipping screens with the buttons is visible
  - white background: if the panel shows the opposite (white marks on
    black), the meaning of bit 1 and bit 0 is swapped somewhere
"""

import argparse
import socket
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

WIDTH, HEIGHT = 800, 480
ROW_BYTES = WIDTH // 8

# Same ids, same order, as kScreenIds in firmware/src/pure/screens.h.
SCREEN_IDS = ["home", "time-left", "year-dots", "night-sky", "family-week", "weather", "word"]

# How long the device should sleep after a successful request. Short, so a
# timer wake (and the 304 answer that follows) can be watched without waiting.
NEXT_WAKE_SECONDS = 120


def fill_rect(frame, left, top, width, height):
    """Paints a black rectangle into the frame.

    The frame is a flat array of bytes, one bit per pixel, most significant
    bit first: pixel x of row y lives in byte (y * 100 + x // 8), at bit
    (7 - x % 8). Bit 1 is white, so painting black means CLEARING the bit:
    AND with a mask that has a 0 only at that position.
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


# Built once at start-up. The ETag changes only when the image would change;
# here that is never, so every repeated request for a screen gets a 304.
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

        # send_response() also adds the Date header, which the device uses
        # to set its clock.
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
    """Finds this computer's address in the local network.

    Trick: "connecting" a UDP socket sends nothing, but it makes the operating
    system choose the network interface it would use, and that interface's
    address can then be read back.
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

    # "0.0.0.0" means: accept connections on every network interface, not
    # only from this computer itself.
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
