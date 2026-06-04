temps_utile-
============

# 6 x clock generator.

![](https://c1.staticflickr.com/1/628/20400765240_149a3ea220_b.jpg)

... a fairly simple breakout board for teensy 3.1/3.2 (and now **teensy 4.0**), focused on **clock sequencing** 

(the name may suggest as much ... it was stolen from M. **[Louis Lapicque](https://en.wikipedia.org/wiki/Louis_Lapicque)** (see: idem, 1907: Sur l'excitation par décharge de condensateurs; détermination directe de la durée et de la quantité utiles. _Comptes Rendus Soc. Biol._ (Paris) 62, 701-704).

---

### what's new in this fork (`v1.4`)

This is a fork of mxmxmx's original [temps_utile-](https://github.com/mxmxmx/temps_utile-).
The 7 per-channel clock/trigger modes are unchanged. The fork adds Teensy 4.0
support and, in doing so, drops channel 4's analog CV mode:

- **Teensy 4.0 support.** The firmware now builds and runs on a Teensy 4.0
  (IMXRT1062) dropped into the same module PCB — same jacks, same ICs, no
  board respin.
- **Channel 4's `DAC` / CV mode is removed — on *every* build, T3.2 included.**
  The Teensy 4.0 has no on-chip DAC, so the analog-CV channel mode (random /
  binary / "Turing" / logistic / sequencer-arpeggiator CV) was cut from the
  shared firmware. Channel 4 is now a **plain 6th clock/gate output** on both
  MCUs. On Teensy 3.1/3.2 the on-chip DAC (pin `A14`) is still the physical
  driver, but it is only ever toggled between two levels (a gate), never used
  for CV. **If you relied on channel 4 as a CV/pitch output, stay on the
  upstream firmware.**
- See **[Channel 4: DAC or gate — jumper + pogo pin](#channel-4-dac-or-gate--jumper--pogo-pin)**
  below: a hardware step required to use channel 4 as a digital gate on either MCU.

**Direction of the fork.** Losing the channel-4 CV output is not the end state —
it is a consequence of **winding down Teensy 3.1/3.2 support in future releases**
and moving the module onto the Teensy 4.0. The far larger headroom of the
IMXRT1062 (clock, RAM, flash) is what makes that worthwhile: the freed channel
and the freed resources will be **reinvested into expanded module
functionality** in subsequent releases, more than compensating for the single CV
output dropped here. Teensy 3.2 builds remain available for now for users who
need the legacy hardware.


### hardware basics, in brief:

- **teensy 3.1/3.2** @ 120MHz (or **teensy 4.0**), w/ 128x64 OLED
- trigger-to-output **latency** < 100us.
- **2 clock inputs** (> 100k input impedance; threshold ~ 2.5V)
- **4 CV inputs** (100k input impedance, -/+ 5V, assignable to (almost) any parameter)
- **6 clock outputs**, all digital gates (~10V). On **T3.2** channel 4's gate is driven from the on-chip DAC pin `A14`; on **T4.0** from a GPIO. (Upstream's ±5V CV on channel 4 is **not** in this fork — see below.)
- two encoders w/ switches; 2 tactile buttons.
- 14HP, ~ 25 mm Depth

### firmware: 

- **7 modes, selectable per channel:** 

  - trigger sequencer/sequence editor
  - clock division/multiplication
  - LFSR
  - random w/ threshold
  - euclidian
  - logic (AND, OR, XOR, NAND, NOR, XNOR)
  - burst

  (upstream's 8th mode — `DAC` on channel 4: random / binary / "Turing" /
  logistic / sequencer-arpeggiator CV — has been **removed in this fork**;
  channel 4 is now a plain gate.)


### Channel 4: DAC or gate — `CLK/DAC` selector + pin-29/26 pogo

In this fork channel 4 is always a **gate** (no CV mode). What varies is *which
physical Teensy pin* carries that gate to the ch4 jack:

- **The `A14` DAC pin** — driven only between two levels (gate, never CV). Works
  on **teensy 3.1/3.2 only** (T4.0 has no DAC). This is the stock mxmxmx path.
- **The digital `CLK4` GPIO** — a plain GPIO gate. The **only** option on
  **teensy 4.0**; available on T3.2 too.

A selector on the module PCB (silk **`CLK/DAC`**; Jakplugg calls it **`J2`**,
digital position **`CL4`**) routes one of the two to the jack. Note: on the
original mxmxmx board this selector is often a **soldered wire bridge**, not a
removable header — factory-bridged to the `DAC` side.

> ⚠️ **To run channel 4 from the digital `CLK4` gate (mandatory on Teensy 4.0;
> needed on T3.2 if you want the GPIO path), you must do two things — otherwise
> nothing comes out of jack 4:**
>
> 1. **Select the `CLK` side of `CLK/DAC`.** If it's a removable jumper, move it.
>    **If it's a soldered wire bridge (as on most mxmxmx boards), de-solder it
>    and re-bridge to the `CLK` side.**
> 2. **Contact the `CLK4` bottom pad with a pogo pin.** `CLK4` comes out on an
>    extra Teensy pin that exists only as a pad on the **bottom** of the Teensy,
>    not a normal edge pin. **The original mxmxmx board leaves this pad
>    unpopulated** (the build guide says "ignore the pads labelled `29`") — this
>    fork uses it again, so if no pogo / wire is fitted you must **add one.**
>
> | MCU            | bottom-pad pin for the `CLK4` gate |
> |----------------|------------------------------------|
> | teensy 3.1/3.2 | **pin 29**                         |
> | teensy 4.0     | **pin 26**                         |
>
> **The two boards' bottom-pad layouts differ — ring the pad out by continuity
> before soldering the pogo. Do not trust visual alignment.**

On **teensy 3.1/3.2** the firmware drives both the `A14` gate and the pin-29
`CLK4` gate in parallel, so if you stay on the `DAC`/`A14` side everything works
with no board mod. On **teensy 4.0** there is no `A14`, so the `CLK` selector
position **and** the pin-26 pogo are both mandatory.

> **Note — the other jumper (`CV trim`).** The original board also carries a
> separate **`CV trim`** jumper (the `AREF` / −5 V offset reference for the
> **CV inputs**, by the `LM4040-5` / `79L05`), unrelated to channel 4. It was
> hard-wired away in board **rev 1.c**, which is why later boards and the
> Jakplugg variant don't have it.


### build guide: [see here](https://github.com/mxmxmx/temps_utile-/wiki/build-it)

