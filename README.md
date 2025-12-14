# aLora — Private, Infrastructure-Independent Messaging over LoRa Mesh

> **Status:** Early prototype, now with interactive compose/send, on-device deduplication, and OLED bring-up hardening. Core reliability and security work continues.

## AI authoring note

This project is designed and implemented by **GPT-5.1-Codex-Max** (me), operating under the direct direction and approval of **Járn**. Every code change, architectural decision, and progress note is authored by the AI, with Járn providing oversight and final sign-off.

## Why this project exists

Modern messengers assume **GSM**, **internet**, or **satellite** connectivity and centralized infrastructure. In many real-world scenarios—remote travel, disasters, outages, censorship, or simply “no coverage”—those assumptions fail.

**aLora** is a firmware project that delivers **device-to-device messaging (and later voice memos)** over a **LoRa mesh** with:

* **Zero dependency** on GSM/internet/satellite.
* **DM-first** user experience (direct, private conversations).
* **Deterministic mesh behavior** (no broadcast storms, no duplicate re-forward loops).
* **End-to-end privacy** via **AES-256** after an explicit peer approval flow.
* **A modern, minimalist UI** suitable for small handheld devices (portrait OLED today; TFT later).

## Project goals

### Core goals (v1)

1. **Infrastructure independence**: messaging without GSM/internet/satellite.
2. **Lightweight**: low RAM/flash footprint, low CPU overhead, and minimal airtime.
3. **DM-first**: direct chats are the primary workflow; group messaging may be added later.
4. **Route-aware delivery**:

   * Prefer **unicast along a learned route**.
   * If delivery fails, initiate **controlled discovery** to re-learn a viable route.
5. **Delivery confirmation**: a “double-tick” style receipt is mandatory for DM messages.
6. **Mesh participation**: devices forward traffic for others (relay behavior) even if not paired.
7. **No duplicate forwards**: a node must never forward the **same message twice**.

### Security goals

* **Peer approval**: private messaging starts only after the recipient accepts a pairing request.
* **E2E encryption**: once paired, DM payloads are encrypted using **AES-256**.
* **Replay protection**: message IDs and anti-replay rules prevent stale/duplicated packets from being accepted.

> Note: Mesh relays are not required to decrypt DM payloads; they only forward opaque packets.

### Non-goals (explicit)

* Not a replacement for high-throughput networking.
* Not attempting “perfect anonymity” (radio metadata exists at the PHY/MAC level).
* Not optimized for large broadcast chat rooms in the first milestone.

## High-level design

aLora is built as layered subsystems so the radio/mesh layer, application protocol, security, storage, and UI can evolve independently.

### 1) Mesh foundation

* **LoRaMesher** provides the underlying mesh transport and forwarding mechanics.
* aLora uses DM-oriented patterns on top:

  * Prefer route-aware unicast.
  * Controlled discovery only when needed.
  * Strict deduplication to prevent forwarding loops.

### 2) Application protocol

Each DM message is wrapped with:

* Sender ID
* Recipient ID
* Message ID (monotonic / random-unique)
* Payload type (text now; voice memo later)
* Optional ACK / delivery receipt fields

### 3) Reliability and receipts

* **Delivery receipt (“double tick”)** is required.
* If no receipt arrives within a timeout window:

  1. Retry along current route.
  2. If still unsuccessful, initiate discovery to locate an alternative route.
  3. Continue DM over the new route.

### 4) Pairing + encryption (AES-256)

Pairing is designed for field usability:

1. A device broadcasts a **presence/advertisement** (over LoRa and/or Wi-Fi, configurable).
2. Nearby devices can view it in the UI and send a **pair request**.
3. The recipient must **explicitly accept**.
4. Upon acceptance, both devices derive a shared key and start **E2E AES-256** messaging.

**Important:** The mesh still forwards packets for everyone. Pairing controls **who can read and authoritatively send private messages**, not who can relay traffic.

### 5) UI model

Two UI backends are planned:

* **Portrait OLED (SSD1306/SSD1315)** — current focus.
* **SPI TFT 2.8"** — planned; not implemented yet.

The UI is intentionally small:

* **Chats** (DM list + history)
* **Status** (airtime, TX/RX, queue depth, route/health)
* **Settings** (radio profile, UI options, identity)

Input is primarily via **rotary encoder**, optimized for fast text composition.

## Current implementation status

This repository is in a **prototype stabilization phase**. Latest highlights:

* OLED bring-up hardened (auto I2C address detection, controller selection, boot diagnostics).
* Rotary input integrated for menu navigation and text editing.
* Compose view can now **set destination, move the cursor, edit characters, and send** DMs directly over the mesh.
* Incoming packets are **deduplicated on-device** before reaching the UI/log.
* One-hop delivery receipts are **acknowledged and reflected in the chat list** (checkmark when delivered).
* Basic message persistence (chat tail/history concept).

## Known gaps / next steps

* **Reliable delivery + receipts:** strict double-tick semantics, retry logic, and failure escalation (basic one-hop ACKs are in place; retries are pending).
* **Pairing protocol:** advertisement, request/accept UX, and secure key establishment.
* **AES-256 E2E:** encryption of DM payloads, key storage, replay protection.
* **Routing-aware behavior:** track successful paths and prefer them; fall back to controlled discovery.
* **Airtime discipline:** accurate time-on-air estimates, rate limiting, jitter/backoff.
* **Interoperability contract:** stable on-air format so different devices/brands running aLora can relay consistently.

## Configuration philosophy

aLora is designed to be configured from PlatformIO without hardcoding “device combos” into the codebase.

* **Environment selects the MCU family** (e.g., ESP32-S3 vs ESP32-C3).
* **Build flags select features**:

  * Display backend: OLED / TFT / none
  * OLED controller: SSD1306 / SSD1315 / auto
  * Rotary: enabled/disabled
  * LoRa module profile: SX127x / SX126x (planned)
  * Pinmap selection (board wiring header)
  * Radio defaults (frequency/BW/SF/sync word/power)

This keeps the firmware lightweight while remaining adaptable.

## Roadmap

### Milestone 0 — Stabilize the baseline (in progress)

**Goal:** a reliable build + working OLED UI on the reference device.

* [x] Solid OLED bring-up for **SSD1315** and **SSD1306** (auto address + reset + rotation).
* [x] Lock down LoRaMesher versioning + compatibility wrapper.
* [x] Deterministic dedupe (no re-forward of identical packets).
* [x] Basic DM text send/receive across 2–3 nodes.

### Milestone 1 — DM reliability (double-tick)

**Goal:** DM messaging feels dependable.

* [ ] Delivery receipts (ACK) with strict semantics.
* [ ] Retry policy (bounded, airtime-aware).
* [ ] Failure escalation: broadcast discovery only when necessary.
* [ ] Status page: airtime + health metrics.

### Milestone 2 — Pairing + AES-256 E2E

**Goal:** private DMs are secure and user-controlled.

* [ ] Presence broadcast (LoRa and/or Wi-Fi).
* [ ] Pair request/accept workflow in UI.
* [ ] Key derivation and secure storage.
* [ ] AES-256 encryption for DM payloads.
* [ ] Replay protection + message authenticity.

### Milestone 3 — Usability upgrades

**Goal:** composing messages on-device is fast and comfortable.

* [ ] Improved rotary text entry (groups, shortcuts, word suggestions).
* [ ] Better chat history browsing.
* [ ] Contact list + recently seen devices.

### Milestone 4 — Voice memos (optional)

**Goal:** short voice messages over constrained LoRa links.

* [ ] Define voice memo format and fragmentation.
* [ ] Lightweight codec choice and rate control.
* [ ] Store-and-forward rules and progress UI.

### Milestone 5 — TFT UI backend

**Goal:** a “master device” experience on a larger screen.

* [ ] SPI TFT backend.
* [ ] UI layout optimized for touch/knob hybrid (if applicable).

## Collaboration principles

* **Interoperability first:** changes to the on-air format require versioning.
* **Airtime is precious:** no unnecessary broadcasts, bounded retries, and strict dedupe.
* **Security by design:** pairing and encryption are first-class, not optional patches.

## Hardware testing support for Járn

The project is in an **early prototype** stage and needs more hardware to broaden radio testing. Supporting Járn’s field work (new modules, antennas, and test gear) will accelerate coverage, interference tolerance, and platform compatibility work. Contributions can be sent to:

* **BTC:** `bc1qpp3ptl2hhrdcn86e9vujanneg9frsrgd4czq7q`
* **ETH:** `0x2FF7bf0D92aC98d4036a89D1612021473cB82772`
* **XRP:** `rJ7LFDzsj9NuoCWYmc3LnVpJde13ZwG635`
* **USDT (ETH network):** `0x2FF7bf0D92aC98d4036a89D1612021473cB82772`
* **Solana:** `Huzw3NSc73hPUWNNJcS1r2uKT61Ekqg2YN3ihshie8W5`

**Why now?** Without diverse hardware, validating coverage, noise resilience, and cross-chip behavior takes longer. Your support lets Járn purchase boards, antennas, and test equipment while I (GPT-5.1-Codex-Max) quickly iterate on the firmware using field feedback.
* **Modularity:** radio/mesh, protocol, security, storage, and UI remain separable.

## Repository structure (intended)

* `src/`

  * `net_*` — mesh integration, reliability, dedupe
  * `ui_*` — UI state machine and pages
  * `display_*` — display backends
  * `input_*` — rotary input
  * `storage_*` — settings + chat history
  * `security_*` — (planned) pairing + AES-256
* `include/`

  * `pinmaps/` — board wiring headers
  * `config/` — build-time feature configuration

## Disclaimer

This is experimental firmware. Expect breaking changes while the protocol, security model, and UI are stabilized.
