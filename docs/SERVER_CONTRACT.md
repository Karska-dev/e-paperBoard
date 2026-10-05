# Server contract

What the device asks for and what it expects back. This is the complete interface between the firmware and the server.

**Status: proposed (v0).** The server does not exist yet. This document describes what the firmware in this repository implements; change both together.

## Request

```
GET {server}/screen?id=home&bat_mv=3940&bat_pct=68&low=0&fw=v0.1.0&wake=timer
If-None-Match: "abc123"          (only when the device already shows this screen)
```

| Parameter | Example | Meaning |
| --- | --- | --- |
| `id` | `home` | Which screen. One of: `home`, `time-left`, `year-dots`, `night-sky`, `family-week`, `weather`, `word` |
| `bat_mv` | `3940` | Battery voltage in millivolts |
| `bat_pct` | `68` | Battery charge, 0 to 100 |
| `low` | `0` | `1` when the battery is low (on at 8 % or less, off again at 12 % or more) |
| `fw` | `v0.1.0` | Firmware version, from `git describe` |
| `wake` | `timer` | Why the device woke: `boot`, `timer`, `prev`, `home` or `next` |

All values use only `A-Z a-z 0-9 . _ -`, so nothing is percent-encoded.

The device sends `If-None-Match` only when it asks for the screen it is already showing. After a button press it asks for a different screen and sends no ETag.

## Response: the image changed (or no ETag was sent)

```
HTTP/1.1 200 OK
Content-Type: application/octet-stream
Content-Length: 48000
ETag: "def456"
Date: Mon, 05 Oct 2026 15:07:00 GMT
X-Next-Wake: 1800

<48,000 bytes>
```

**Body:** a raw 1-bit bitmap, exactly 48,000 bytes.

- 800 × 480 pixels, rows top to bottom, pixels left to right
- 8 pixels per byte, leftmost pixel in the highest bit
- bit `1` = white, bit `0` = black
- no header, no compression

| Header | Required | Rule |
| --- | --- | --- |
| `Content-Length` | **yes** | Must be `48000`. Responses without it (chunked encoding) are rejected. |
| `ETag` | recommended | At most 47 characters including the quotes. Must change whenever the image changes, and must differ between screens. Without it the device redraws on every wake. |
| `Date` | recommended | Standard HTTP date. The device sets its clock from it. Most web servers add it automatically. |
| `X-Next-Wake` | optional | Seconds until the device should wake next. Clamped by the device to 60 … 86,400. Without it the device uses 30 minutes. |

## Response: nothing changed

```
HTTP/1.1 304 Not Modified
ETag: "abc123"
Date: Mon, 05 Oct 2026 15:37:00 GMT
X-Next-Wake: 1800
```

No body. The device leaves the panel untouched and goes back to sleep. Only valid as an answer to a request that carried `If-None-Match`.

## Errors

Any other status, a wrong body size, or no answer at all counts as a failed cycle. The device keeps showing the old image and retries with exponential backoff: after 1, 2, 4, 8, 16 and 32 minutes, then every hour. The first success resets the backoff.

## Transport

Plain `http://` in v0.1, intended for a server on the home network. HTTPS is planned.

## A server for testing

[`tools/test_server.py`](../tools/test_server.py) implements this contract with test images and nothing else. It needs only Python 3:

```sh
python3 tools/test_server.py
```

It prints the address to put into `firmware/include/secrets.h`, then one line per request showing what the device reported and what it was sent.

Each screen gets its own image and its own ETag: a black frame, a black square in the top-left corner (to check orientation), and a row of squares, one for `home`, two for the next screen, and so on. Asking twice for the same screen gives `200` and then `304`.
