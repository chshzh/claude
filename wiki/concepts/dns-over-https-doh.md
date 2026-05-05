---
title: DNS-over-HTTPS (DoH) — Privacy, Office Protection & Setup
created: 2026-05-03
updated: 2026-05-03
type: concept
tags: [networking, dns, encryption, privacy, security, doh]
sources: []
---

## What is DoH?

DNS-over-HTTPS sends your DNS queries inside encrypted HTTPS (port 443) instead of plain-text
UDP (port 53). To a network observer, it looks like regular web traffic — they can see you're
talking to a DNS server, but not *what domains* you're looking up.

## The Problem: Normal DNS is a Privacy Leak

Standard DNS (UDP port 53) transmits every domain you visit in clear text. Anyone on the
network path can read it:

```
You → ISP Router → Office Firewall → DNS Server
         ↑                ↑
    can see all       can see all
```

In an office environment, the IT department's DNS server logs **every domain** every device
resolves. Even if you use HTTPS for the actual website (encrypted), the DNS lookup that
precedes it reveals:

- `google.com` — you searched something
- `reddit.com` — you browsed Reddit
- `linkedin.com` — you visited LinkedIn
- `your-competitor.com` — you researched a competitor
- `health-clinic.no` — you looked up a medical appointment

The DNS log is a complete map of your browsing intent, even when the page content itself is
encrypted.

## How DoH Protects You

```
Normal DNS:
  You ──[DNS query: "reddit.com" (plaintext)]──→ Office DNS Server
                                                      ↓
                                               IT sees: reddit.com

With DoH:
  You ──[TLS-encrypted HTTPS]──→ Cloudflare (1.1.1.1)
                                      ↓
                               Cloudflare sees: reddit.com
                               IT sees: traffic to 1.1.1.1:443
```

The office network sees you're talking to Cloudflare's DoH endpoint — that's it. The actual
DNS query is encrypted inside the TLS tunnel. The office DNS server never sees it.

## Office Threat Model

| What IT can see without DoH | What IT can see with DoH |
|---|---|
| Every domain you resolve | You're connecting to 1.1.1.1:443 |
| Timing of DNS queries | Timing of HTTPS connections to Cloudflare |
| Full DNS query history log | Just a Cloudflare IP |
| Patterns (job hunting, health, etc.) | Nothing domain-specific |

**Caveat:** DoH hides DNS, but not everything.

### What DoH Does NOT Hide

1. **SNI (Server Name Indication)** — the domain you're visiting is still sent in plain text
   during the TLS handshake, unless the server supports **ECH (Encrypted Client Hello)**.
   Most servers still don't (as of 2026). Without ECH, the office firewall sees:
   `TLS ClientHello → SNI: reddit.com`.

2. **IP addresses** — after DNS resolution, your browser connects to an IP. The office firewall
   sees `you → 151.101.1.140:443`. Reverse IP lookup reveals it's Reddit.

3. **Traffic volume and timing** — even encrypted, the pattern of connections (frequency, size,
   timing) can fingerprint sites.

DoH alone closes the DNS leak. For full privacy, combine with a VPN or use a browser with ECH
support (Firefox, Chrome with flags).

## How to Enable DoH

### Option A: Browser-Level (simplest)

**Firefox:** Settings → Privacy & Security → DNS over HTTPS → Max Protection → Cloudflare

**Chrome/Chromium:** Settings → Privacy & Security → Security → Use secure DNS → With: Cloudflare

This only protects browser traffic. Terminal apps, `apt`, `curl`, etc. still use plain DNS.

### Option B: System-Wide with cloudflared (recommended for Linux)

Install:

```bash
# Ubuntu
curl -fsSL https://pkg.cloudflare.com/cloudflare-main.gpg | \
  sudo tee /usr/share/keyrings/cloudflare-main.gpg > /dev/null
echo "deb [signed-by=/usr/share/keyrings/cloudflare-main.gpg] \
  https://pkg.cloudflare.com/cloudflared $(lsb_release -cs) main" | \
  sudo tee /etc/apt/sources.list.d/cloudflared.list
sudo apt update && sudo apt install cloudflared
```

Verify:

```bash
cloudflared --version
```

Configure as a systemd service — create `/etc/systemd/system/cloudflared-doh.service`:

```ini
[Unit]
Description=Cloudflare DoH Proxy
After=network.target

[Service]
ExecStart=/usr/bin/cloudflared proxy-dns --port 5300 --upstream https://dns.cloudflare.com/dns-query
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now cloudflared-doh
```

Point systemd-resolved at it — edit `/etc/systemd/resolved.conf`:

```ini
[Resolve]
DNS=127.0.0.1#5300
DNSOverTLS=no
DNSSEC=yes
Domains=~.
```

```bash
sudo systemctl restart systemd-resolved
```

Verify:

```bash
resolvectl status | grep "DNS Servers"
# → 127.0.0.1#5300

dig +short example.com
# Should resolve via the proxy
```

### Test It's Working

Visit https://1.1.1.1/help in a browser. It shows whether DoH is active.

For the terminal:

```bash
curl -s https://cloudflare.com/cdn-cgi/trace | grep -i warp
# Should NOT show warp=on (that's for Warp VPN, different thing)
# The fact that DNS resolved means it went through your proxy
```

## DoH vs DoT

| | DNS-over-HTTPS (DoH) | DNS-over-TLS (DoT) |
|---|---|---|
| Port | 443 | 853 |
| Looks like | Regular HTTPS | Dedicated DNS port |
| Blockability | Hard to block (mixes with web) | Easy to block (dedicated port) |
| Best for | Evading firewalls | System-wide DNS on trusted networks |

In an office environment, DoH is preferred because blocking port 443 would break the internet.
Blocking port 853 (DoT) is trivial.

## Limitations in Practice

1. **IT can still block known DoH endpoints** — Cloudflare's `1.1.1.1`, Google's `8.8.8.8`,
   Quad9's `9.9.9.9` are well-known. Some firewalls block these IPs even on port 443.
   Workaround: use a lesser-known DoH provider, or a VPN.

2. **Most office devices are managed** — if the company installed a root CA certificate on your
   machine, they can MITM all TLS traffic, including DoH. Check: Settings → Certificates →
   look for corporate CAs.

3. **ECH is not universal yet** — the SNI in the TLS handshake is still plain text for most
   sites. Firefox enables ECH by default; Chrome requires `chrome://flags/#encrypted-client-hello`.

4. **VPN is the stronger answer** — DoH fixes DNS. A VPN fixes everything (DNS + SNI + IPs +
   traffic patterns). If you need real privacy in an office, use a VPN.

## Related

- [[wireguard-openwrt-china-tunnel]] — WireGuard tunneling, similar encryption principle applied to all traffic
- [[hermes-setup-on-linux-with-nas-nfs-backup]] — this machine's DoH setup can be documented there
