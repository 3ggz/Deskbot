# HOUSING.md — Robot Body Build Guide

## Design Concept

```
      ╔══════════════╗
      ║  ◉  face  ◉  ║  ← Round display (2.1") — the face
      ╚══════╤═══════╝
             │          ← XiaoR pan-tilt bracket (neck)
       ╔═════╧═════╗
       ║  [  body  ]║  ← Speaker grille + LED strip base
       ╚═══════════╝
```

Target aesthetic: Small, round-headed, friendly. Think a tiny robot from a Pixar short.
No arms needed for v1. Head movement (pan/tilt) + body wiggle = enough personality.

---

## Option A — DIY with Everyday Materials (Start Here)

Prototype with these first. Cost: ~$5–10. Skill: Dremel + spray paint.

### Head Options
| Material | Size to use | Where to get |
|---|---|---|
| PVC end cap | 3–4" diameter | Hardware store ~$2 |
| Round plastic food container lid | 3–4" diameter | Kitchen or dollar store |
| Tennis ball (cut open) | ~2.5" | Sporting goods |
| Small plastic ornament ball | 3–4" | Craft store |

**For the display hole:** Use a Dremel with a cutting wheel. Mark a circle slightly smaller than the display bezel. Cut slowly. Sand edges smooth. Test fit before painting.

### Body Options
| Material | Notes |
|---|---|
| PVC pipe section (3–4" wide, 3–4" tall) | Best proportions, easy to Dremel, spray paints perfectly |
| Round Tupperware/food container | Already closed top/bottom, easy to cut |
| Cylindrical cardboard tube (mailing tube) | Quick prototype, not durable |
| Gutted small Bluetooth speaker | Already has speaker grille cutouts, robot-body silhouette |

### Finishing
1. Sand all edges smooth (120 grit → 220 grit)
2. Spray primer (grey or white) — 2 light coats
3. Spray matte white or chosen color — 2–3 light coats
4. Optional: accent color for details (eyes rim, seams)

---

## Option B — 3D Printing Service (Best Long-Term Result)

Do this after prototyping with Option A — you'll know exact dimensions that work.

### Where to print
| Service | Price est. | Speed |
|---|---|---|
| JLCPCB (jlcpcb.com) | ~$10–20 | 2–3 weeks (ships from China) |
| PCBWay | ~$15–25 | 2–3 weeks |
| UPS Store (in-person) | ~$20–40 | Same day or next day |
| Local library makerspace | Free–$5 | Varies |
| Craftcloud.com | ~$25–50 | 1–2 weeks (local printers) |

### Free STL files to use
Search these sites for "desktop companion robot" or "robot head round display":
- **Printables.com** — best quality community files
- **Thingiverse.com** — largest selection
- **Cults3D.com** — more curated designs

Design requirements to filter by:
- Fits 2.1" round display (~50mm diameter cutout)
- Pan-tilt servo mount on bottom of head
- Hollow interior for cable routing
- ~80–120mm total head height

---

## Neck Mount — XiaoR Pan-Tilt Kit Integration

The XiaoR pan-tilt bracket has M3 mounting holes on top.
The robot head attaches to these holes with M3 screws.

For DIY head (PVC/plastic):
1. Mark hole positions from bracket onto head bottom
2. Drill M3 holes (3mm drill bit)
3. Use M3 x 8mm screws + washers to secure

Cable routing:
- Display ribbon/wires route through a small slot in the head bottom
- Servo wires from pan-tilt route to PCA9685 in body
- All wires bundle through the neck column into the body shell

---

## Body Interior Layout

```
Body shell (top view):
┌─────────────────────────┐
│  [PCM5102 DAC]          │
│  [Speaker → grille →]   │
│  [LED strip rim]        │
│  [Cable bundle to Pi]   │
└─────────────────────────┘
```

**What lives in the robot body:**
- PCM5102 DAC board
- Speaker (CQRobot 3W)
- PCA9685 servo driver
- LED strip (around the base rim)

**What lives in the Pi CanaKit case:**
- Raspberry Pi 5 8GB
- All main compute and power

**Cable bundle between them:**
- SPI (display): 6 wires
- I²S audio out (DAC): 3 wires
- I²S audio in (mic): 4 wires
- I²C (PCA9685): 2 wires + power
- 5V + GND for servo PSU
- USB or GPIO for LED strip

Use a **braided cable sleeve** over the cable bundle for a clean look.
Route through the desk or along the back — keep the front clean.

---

## Dimensions Reference

| Component | Dimensions |
|---|---|
| Waveshare 2.1" round display | ~55mm diameter, ~5mm thick |
| XiaoR pan-tilt base footprint | ~60mm × 60mm |
| SG90 servo | 23mm × 12.5mm × 29mm |
| PCA9685 board | ~62mm × 26mm |
| PCM5102 board | ~23mm × 17mm |
| CQRobot speaker | ~40mm diameter |

**Recommended body shell minimum interior:** 80mm wide × 80mm tall
**Recommended head shell minimum interior:** 60mm × 60mm × 50mm deep

---

## Build Order
1. ✅ Get all electronics working on breadboard first (no shell)
2. Measure real component positions when everything is working
3. Build prototype shell (PVC/container) — fit test all components
4. Route and label all cables
5. If happy with prototype → send to 3D print service for clean version
6. Reassemble in printed shell
