---
name: Pokémon Platinum Local Co-op
description: Native two-player launcher with a restrained handheld-game character.
colors:
  casing: "#142c3b"
  action: "#223f4e"
  action-hover: "#2d5261"
  action-focus: "#325666"
  action-pressed: "#102633"
  action-border: "#5b7b89"
  hover-border: "#b6c9cd"
  disabled-bg: "#1a3443"
  disabled-text: "#8ba1aa"
  disabled-border: "#3e5966"
  ivory: "#f3eee0"
  ink: "#213846"
  muted: "#c0d2d7"
  focus-gold: "#f5cf67"
  bright-text: "#fff9e9"
  coral: "#b83f35"
  coral-hover: "#cc4b3e"
  coral-pressed: "#92352e"
  coral-border: "#e78d7a"
  panel-heading: "#8b392e"
  dialog: "#183442"
  field: "#0e2532"
  border: "#8fa5aa"
  scroll-handle: "#72909c"
  error: "#ffb0a3"
  exit-text: "#e5b4a8"
  player-one: "#f59c89"
  player-two: "#91dce0"
  play-bg: "#0e2330"
  play-divider: "#3b5966"
typography:
  display:
    fontFamily: "Silkscreen"
    fontSize: "30px"
  title:
    fontFamily: "Silkscreen"
    fontSize: "14px"
  body:
    fontFamily: "QApplication::font()"
    fontSize: "18px"
  label:
    fontFamily: "QApplication::font()"
    fontSize: "15px"
  selected-rom:
    fontFamily: "QApplication::font()"
    fontSize: "21px"
    fontWeight: 600
rounded:
  field: "4px"
  action: "8px"
  panel: "12px"
spacing:
  field: "6px"
  grid-row: "10px"
  action: "12px"
  group: "14px"
  section: "20px"
  canvas-y: "24px"
  canvas-x: "32px"
components:
  button-primary:
    backgroundColor: "{colors.coral}"
    textColor: "{colors.bright-text}"
    typography: "{typography.selected-rom}"
    rounded: "{rounded.action}"
    padding: "{spacing.action}"
  button-primary-hover:
    backgroundColor: "{colors.coral-hover}"
  button-primary-active:
    backgroundColor: "{colors.coral-pressed}"
  button-secondary:
    backgroundColor: "{colors.action}"
    textColor: "{colors.ivory}"
    typography: "{typography.body}"
    rounded: "{rounded.action}"
    padding: "{spacing.action}"
  button-secondary-hover:
    backgroundColor: "{colors.action-hover}"
  button-secondary-focus:
    backgroundColor: "{colors.action-focus}"
    textColor: "{colors.bright-text}"
  button-secondary-active:
    backgroundColor: "{colors.action-pressed}"
  button-secondary-disabled:
    backgroundColor: "{colors.disabled-bg}"
    textColor: "{colors.disabled-text}"
  game-panel:
    backgroundColor: "{colors.ivory}"
    textColor: "{colors.ink}"
    rounded: "{rounded.panel}"
    padding: "{spacing.section}"
  field:
    backgroundColor: "{colors.field}"
    textColor: "{colors.ivory}"
    rounded: "{rounded.field}"
    padding: "{spacing.field}"
  alpha-badge:
    textColor: "{colors.muted}"
    padding: "{spacing.field}"
---

# Design System: Pokémon Platinum Local Co-op

## Overview

**Creative North Star: "The Shared Handheld"**

A modern native launcher with a stronger Pokémon/retro character, as selected by the user. Graphite-teal casing, warm ivory information and compact pixel headings recall a handheld game menu while platform body text keeps German labels readable.

The interface prioritizes two-player couch use: one coral start action, real ROM/controller/session information, and a consistent gold selection edge. Retro character comes from type, color and original geometric icons; there is no Nintendo artwork or ripped game font.

**Key Characteristics:**
- Native Qt desktop utility with German labels.
- Pixel headings paired with readable platform controls.
- Coral primary action and gold keyboard/controller focus.
- Honest status and independent player identifiers.

## Colors

The palette uses dark teal casing, ivory information surfaces and warm action accents. Frontmatter values are normative; `launcher/Theme.h` and `launcher/main.cpp` implement them.

### Primary

- **Coral:** start action, with separate hover, pressed and border tokens.
- **Selection Gold:** display/section headings and the shared focus edge.

### Secondary

- **Player Coral / Player Cyan:** player headings and touch cursors in the play view. These distinguish players, not readiness.

### Neutral

- **Graphite Casing:** menu canvas; darker fields and a related dialog surface create depth.
- **Warm Ivory / Dark Ink:** game information panel and tooltips; ivory also supplies light control text.
- **Mist Text:** subtitle, help and normal session status. Error, exit and disabled tokens remain distinct.
- **Slate Borders:** panel/field edges, control edges and scrollbar thumb; hover borders brighten.

**The State Truth Rule.** Color supports actual status text; it never asserts a connection, selected ROM or running session by decoration alone.

## Typography

Silkscreen is embedded through Qt resources for the product title, section headings and player headings. If loading fails, Qt falls back to the application font. Body, buttons, fields and explanatory text use `QApplication::font()`; its family is platform-dependent. No custom line height or letter spacing is imposed.

- **Display:** default title size from the frontmatter; compact title (23px).
- **Title:** section headings from the frontmatter; play-view player headings (18px).
- **Body / Label:** controls use body size; subtitle, controller details, navigation and session status use label size.
- **Selected ROM:** emphasized size and weight from the frontmatter; the start button shares this treatment.
- **Play details:** controller state (14px); wrapped help uses native point sizing (14pt).

At the large-window threshold, menu type sizes double. Play-view type retains its explicit sizes.

The unmodified `Silkscreen-Regular.ttf` comes from [Google Fonts](https://github.com/google/fonts/tree/main/ofl/silkscreen), downloaded on 2026-10-16. It is licensed under SIL Open Font License 1.1, copyright 2001 The Silkscreen Project Authors; the license is in `assets/fonts/OFL.txt`. SHA-256: `c845473330b94c2079ce9af01c51ac8ba2d99c24f4d14c039843bbb8e642ebd8`. See `assets/fonts/README.md` for the exact download URL. It is not a Nintendo font.

## Layout

The native resizable window uses a centered, vertically scrollable menu with outer margins (32px horizontal, 24px vertical), a title/subtitle header, game information/start column and eight-action menu. The normal content maximum is (1440px); columns have stretch weights (5:6). Group spacing is (14px), section spacing (20px), action grid spacing (12px horizontal, 10px vertical).

Below window width (1040px), the working columns stack. Below (760px), actions become one column and the title uses compact sizing. Horizontal scrolling is suppressed; keyboard/controller focus scrolls its action into view. At width at least (2800px) **and** height at least (1600px), the content maximum becomes (2600px), section/panel spacing (32px), action grid gaps (20px horizontal, 16px vertical), icon size (40px) and menu font/padding scale doubles. Otherwise icons are (24px). Radii and border widths do not double.

The start widget has minimum height (74px); other menu actions (56px), alongside theme content minima and padding. Qt size hints determine the resulting widget size. Settings remain native dialogs with platform window chrome and native file pickers. No browser breakpoints or mobile navigation are introduced.

The play view supports left/right or top/bottom players and the existing DS screen arrangements. Player areas inset (12px), reserve heading space (64px), and size wrapped help before allocating screen space. Framebuffers use nearest-neighbor image scaling; player names and touch cursors share their identifier color.

## Elevation & Depth

The implementation is flat: no authored drop shadows, gradients, blur or launch animation. Tonal casing layers, contrasting ivory information and crisp borders separate regions. Native platform window decoration remains outside this theme.

## Shapes

Fields and scrollbar thumbs use the small radius; action buttons use the medium radius; the ivory information panel uses the large radius. Panels and buttons have crisp borders (2px); default fields have a thinner border (1px). The ALPHA badge is a plain rectangular outline. Original line icons use simple cartridge, controller, screen, save and utility geometry.

## Components

### Buttons

The coral primary action is “Koop-Spiel starten”; secondary actions use the dark action surface and left-aligned icon/label. Hover brightens the surface and border. Focus uses a gold border (2px); secondary focus also lightens the background, while the primary keeps its coral surface. Pressed state darkens; secondary disabled state uses muted text, fill and border. Exit has its own muted warm text. State changes are immediate, without authored transitions.

### Cards / Containers

The ivory game panel contains the actual selected ROM or empty state, followed by controller assignments or instructions. Its internal padding is normal section spacing and grows at the large-window threshold. The panel heading uses the dark warm heading token, preserving contrast on ivory.

### Inputs / Fields

Line edits, combo boxes and log text use dark recessed surfaces, small rounded corners and field padding. Focus replaces the default field edge with gold (2px). Text selection uses gold with dark ink. Combo popups use the dialog surface and action-focus selection fill. Keep Qt editing, selection and platform picker behavior.

### Navigation

Tab order starts with the primary action and follows the eight menu actions. The mapped controller moves focus through the same actions and opens the focused action. The on-screen instruction remains “Steuerkreuz: Auswahl · A: Öffnen · Tastatur: Tab / Leertaste”. Dialog dismissal labels are “Abbrechen” and “Schließen”; native focus behavior remains in charge of dialogs.

### Status / Player Chrome

ALPHA is a release-stage badge, not a live-status chip. Session errors use the error token; normal status uses muted text. Play chrome labels Spieler 1/2, actual controller connection state and wrapped control help. Red/green homebrew framebuffers in the review captures are test content from emulator processes, not Pokémon screenshots or launcher decorations.

## Do's and Don'ts

### Do:
- **Do** use Silkscreen only for headings and retain the platform font for controls and paragraphs.
- **Do** keep the same gold focus edge across actions and fields.
- **Do** wrap German labels and keep focused menu actions visible when scrolling.
- **Do** derive ROM, controller and session text from actual state.
- **Do** use original geometric icons and user-supplied game content only.

### Don't:
- **Don't** add Nintendo artwork, sprites, logos, ripped fonts or bundled commercial game assets.
- **Don't** turn this native desktop utility into a web or mobile interface.
- **Don't** replace state text with decorative ready or connection indicators.
- **Don't** imply physical-controller, Bazzite or native-Windows gameplay validation from visual captures.
