"""Checks one iperf3 client run's JSON (-J) against the options it ran with.
   python3 check_json.py RUN.json [the client's options...]
TCP: bytes sent and received > 0, the receiver got what the sender sent
(less what was still in the socket buffers), the stream count is -P's. -n/-k: at least
that much was sent. UDP: the sender's rate within 20% of -b, under 5% loss on
the loopback. --bidir: both directions carried data.
MIT, Copyright (c) 2026 AmiCradle Team."""
import json
import sys


def size(s):
    mult = {"K": 1024, "M": 1024 ** 2, "G": 1024 ** 3}
    return int(float(s[:-1]) * mult[s[-1]]) if s[-1] in mult else int(s)


def rate(s):
    mult = {"K": 1000, "M": 1000 ** 2, "G": 1000 ** 3}
    return float(s[:-1]) * mult[s[-1]] if s[-1] in mult else float(s)


def main():
    run = json.load(open(sys.argv[1]))
    args = sys.argv[2:]
    opt = {}
    for i, a in enumerate(args):
        if a.startswith("-") and i + 1 < len(args) and not args[i + 1].startswith("-"):
            opt[a] = args[i + 1]
        elif a.startswith("-"):
            opt[a] = True
    bad = []
    if "error" in run:
        bad.append("iperf error: " + run["error"])
    end = run.get("end", {})
    streams = int(opt.get("-P", 1))
    got = len(run.get("start", {}).get("connected", []))
    want = streams * (2 if "--bidir" in opt else 1)
    if got != want:
        bad.append(f"{got} streams connected, wanted {want}")
    if "-u" in opt:
        s = end.get("sum") or end.get("sum_sent") or {}
        bps = s.get("bits_per_second", 0)
        if "-b" in opt:
            target = rate(opt["-b"]) * streams
            if not 0.8 * target <= bps <= 1.2 * target:
                bad.append(f"UDP rate {bps:.0f} bit/s, wanted {target:.0f} +-20%")
        lost = s.get("lost_percent", 100)
        if lost > 5:
            bad.append(f"UDP loss {lost}%")
        if s.get("packets", 0) <= 0:
            bad.append("no UDP packets")
    else:
        sent = end.get("sum_sent", {}).get("bytes", 0)
        recv = end.get("sum_received", {}).get("bytes", 0)
        if sent <= 0 or recv <= 0:
            bad.append(f"TCP bytes sent {sent}, received {recv}")
        # what is still in the socket buffers when the test stops is sent
        # but never counted as received (as with any iperf3): allow 10%
        # or 4 MB, whichever is more
        # and the sender's tally can stop a block before the receiver's
        blk = size(opt.get("-l", "128K")) * want
        if recv > sent + blk or sent - recv > max(sent // 10, 4 << 20):
            bad.append(f"sent {sent} but received {recv}")
        if "-n" in opt and sent < size(opt["-n"]):
            bad.append(f"sent {sent}, less than -n {opt['-n']}")
        if "-k" in opt and sent < int(opt["-k"]) * size(opt.get("-l", "128K")):
            bad.append(f"sent {sent}, less than -k {opt['-k']} blocks")
        if "--bidir" in opt:
            for k in ("sum_sent_bidir_reverse", "sum_received_bidir_reverse"):
                if end.get(k, {}).get("bytes", 0) <= 0:
                    bad.append(f"--bidir: {k} carried nothing")
    for b in bad:
        print("  " + b)
    return 1 if bad else 0


sys.exit(main())
