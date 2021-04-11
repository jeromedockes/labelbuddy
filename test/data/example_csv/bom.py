from pathlib import Path

f = Path("example2-out.csv")
t = f.read_bytes()
f.write_bytes(b"\xef\xbb\xbf" + t)
