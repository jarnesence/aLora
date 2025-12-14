# aLora UX Scenarios and IA

This document captures primary user scenarios, high-level information architecture, and low-fidelity wireframes for the current aLora handheld UI.

## Core user scenarios

### 1) First-time setup / pairing
- **Goal:** Bring a new device onto the mesh with a trusted peer and confirm messaging works.
- **Main actions:**
  - Power on, confirm splash + self-check completes.
  - Open **Contacts → New** to advertise presence or listen for a peer.
  - Exchange/enter pairing PIN, accept request, and wait for success badge.
  - Send a canned DM to verify delivery receipts (double tick) and radio stats.

### 2) Daily messaging and status
- **Goal:** Quickly send/receive DMs and monitor radio/mesh health during normal use.
- **Main actions:**
  - Land on **Dashboard** carousel tile with last contact preview and radio counters.
  - Rotate to **Chats** (per-contact threads) or **Contacts** roster to start a DM.
  - Use canned-message picker for rapid send; confirm delivery receipts.
  - Check **Status** for airtime bucket, queue depth, RSSI/SNR snapshots.

### 3) Maintenance / tuning
- **Goal:** Keep the device reliable over time and adapt to environment changes.
- **Main actions:**
  - Visit **Settings → Radio** to adjust frequency/BW/SF and power presets.
  - Clear stale pairings or re-run discovery for routes that are underperforming.
  - Review **History** for failed messages, retries, and path changes.
  - Use **Help** to recall pairing steps, retry policy, and indicator meanings.

## Information architecture / menu map

Top-level navigation is the rotary carousel; each tile opens sub-pages:

- **Dashboard**
  - At-a-glance: last DM preview, unread count, delivery tick legend.
  - Indicators: uptime, queue depth, airtime bucket %, TX/RX counters, RSSI/SNR snapshot.
- **Chats**
  - List of recent threads (paired + recently seen peers).
  - Thread view: message list with delivery state, quick replies, resend/forget actions.
- **Contacts**
  - Roster of paired + nearby peers; strength indicator for seen beacons.
  - **New pairing** flow: advertise/listen, PIN entry, accept/decline, success badge.
- **Status**
  - Radio stats: frequency preset, BW/SF, power, sync word, channel utilization.
  - Mesh health: successful deliveries vs retries, discovery triggers, route age.
- **History**
  - Chronological log of sent/received packets, retries, and errors.
  - Filters: by contact, by outcome (delivered/failed/pending), by timeframe.
- **Scheduler**
  - Upcoming automated sends (e.g., beacon or status check-ins) with enable/disable.
  - Quick add/edit cadence (interval, destination, payload template).
- **Settings / Profile**
  - Identity: callsign/handle, device ID display, beacon visibility.
  - Radio: power profile, region preset, LoRa parameters, duty-cycle guardrails.
  - UI: brightness, haptic/LED cues, rotary sensitivity, language (future).
- **Help**
  - On-device quickstart, pairing checklist, delivery receipt legend, error codes.

## Wireframe sketches (low fidelity)

Monospace sketches to illustrate layout intent; actual rendering follows OLED constraints.

### Dashboard
```
+----------------------+
| aLora ▸ Dashboard    |
| Last DM: [Ari] ...✓✓ |
| Unread: 2   Queue: 1 |
| Airtime: 72%  Uptime |
| TX:34 RX:36  RSSI:-85|
| [Chats] [Contacts]   |
+----------------------+
```

### Chats / Thread
```
+----------------------+
| Chats ▸ Ari          |
| < prev   type to send|
| -- -- -- -- -- -- -- |
| 10:21 You: On route ✓|
| 10:23 Ari: Copy     |
| [Canned ▸] [Send]    |
+----------------------+
```

### Contacts / Pairing
```
+----------------------+
| Contacts             |
| Paired: Ari ✓  Bea ✓ |
| Nearby: Vox (SNR:-6) |
| [New ▸] Advertise    |
| PIN: 4821  Accept    |
| Status: Linked ✓✓    |
+----------------------+
```

### Scheduler
```
+----------------------+
| Scheduler            |
| Beacon ▸ 15m  ON     |
| Status DM ▸ 2h OFF   |
| Add: dst ▸ Ari       |
|      every ▸ 30m     |
| [Save] [Disable all] |
+----------------------+
```

### History
```
+----------------------+
| History  Filter: All |
| 10:24 DM→Ari ✓✓      |
| 10:22 DM→Bea retry 1 |
| 10:20 Disc→Ari ✓     |
| 10:18 DM→Vox failed  |
| [By contact ▸]       |
+----------------------+
```

### Settings / Profile
```
+----------------------+
| Settings             |
| Profile: callsign ▸  |
| Radio: EU868 ▸ 14dBm |
| Sync: 0x12  BW:125k  |
| UI: bright ▸ med     |
| [Save] [Back]        |
+----------------------+
```

### Help
```
+----------------------+
| Help                 |
| Quickstart ▸         |
| Pairing steps ▸      |
| Receipt legend ▸     |
| Error codes ▸        |
| [Back]               |
+----------------------+
```
