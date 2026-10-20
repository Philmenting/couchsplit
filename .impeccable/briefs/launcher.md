# Launcher visual refinement — user-selected retro direction

User request: make the existing launcher modern and attractive. Confirmed preference: “Stärker im Pokémon-/Retro-Stil”. Preserve the existing native Qt desktop application and its workflows; no website or new application. English and German launcher language, two-player couch co-op, Linux/Zorin/Bazzite and Windows. Safeguards for saves, assignment, session/ROM mismatch and shutdown remain mandatory.

Mode: Operate. First viewport at 1280×720 shows product title, actual selected ROM / empty state, controller assignment / empty state, one primary start action, all eight setup/management actions and real session status. Narrow windows stack the two working areas and keep focus-scrolling. 4K scales controls/text for couch use.

Implementation directly in native widgets, no generated image comp and no stored global build preference. The visual system refines the actual interface into a restrained handheld-game vocabulary: graphite/teal casing, warm ivory game information, coral-red start, gold focus, embedded open-license pixel lettering for headings, native system font for body/control text. Icons are our own simple native geometry. No Nintendo artwork, ROM imagery, sprites, logo/font rips, fake gameplay or false ready/connection indicators.

Signature interaction: controller/keyboard focus is a crisp gold selection outline, consistent for every action. No decorative launch animation, no delay before menus can be used. Player chrome uses coral/cyan identifiers. Existing controller ownership and input code remain unchanged.

Required screenshots: empty and prepared main menu at 1280×720; narrow 640×360 (first screen plus focused exit); 1920×1080 and 3840×2160; existing DS player layouts / wrapped help; one normal settings dialog. Native Qt screenshots, not HTML. No web detector applies to this C++/Qt surface. Inspect in at most two batched capture rounds, with any fixes grouped between.

Verification: native CTest UI/controller/focus/session regressions and full Python suite; source-font embedding checked by native UI test; labels/icons must fit. Before binary distribution require native Linux/Windows builds with full matching source. Actual Pokémon/physical-device/Bazzite testing is not implied by styling checks.


## Bilingual setup extension — 19 October 2026

Narrow code-led extension in Operate mode; retain the existing native retro identity. The first interactive launch, including an existing profile without a language choice, opens a bilingual Language / Sprache dialog before ROM setup. Offer exactly Deutsch and English, initially focused from the OS locale; remember only an explicit choice. Cancel exits setup without writing preferences. Saved valid choices bypass the dialog. Settings offers Language / Sprache and applies the accepted change immediately, including the menu and play help, while active emulator workers keep running. The game's ROM language is independent.

First viewport: bilingual title, short explanation, two equally sized language buttons, change-later hint and explicit Cancel. Signature interaction remains the gold keyboard focus with immediate native button activation. No animation or setup delay. Use a scrolling settings form on compact displays; wrapped guidance stays readable and OK/Cancel remain visible. Preserve legacy JSON fields on initial choice and fail without overwriting corrupt settings or losing a prior language when persistence fails. No new dependencies, font or token system.

Verification: DE/EN production menu and help at 640×360, 720p, 768p, 1080p and 4K; first-choice/persistence/cancel/legacy/corruption/write-failure tests; active fixture session switch without worker restart; native Linux/Windows CI before publication. Native Qt skips the HTML/CSS detector. Captures: `.impeccable/review/languages/`. No new visual world or comp approval required for this scoped extension. Global design-system sidecar drift predates this task and is not repaired here.


## CouchSplit naming — 19 October 2026

The user delegates a new project/repository name emphasizing couch co-op and split screen, without tying the identity to Platinum. Chosen name: **CouchSplit**, repository `Philmenting/couchsplit`. Narrow code-led brand/copy update in Operate mode; the existing palette, fonts, geometric icons and interaction behavior remain authoritative. First viewport title becomes “CouchSplit / Local Co-op”; keep the existing native hierarchy and actual session information. English README with German summary makes current Project PM / Pokémon Platinum support explicit; the new name does not claim additional game support.

Visible application/window/dialog names, desktop/start menu entries, installer labels, current download filenames and repo links use CouchSplit. Preserve legacy application/organization identifiers, profile roots, binary/wrapper/desktop IDs, Debian package ID, Windows AppId/default installation folder and Bazzite runtime ownership for in-place upgrades. Windows removes only its known old named shortcuts during upgrade and creates CouchSplit shortcuts. Never edit other Steam entries or automatically migrate/delete saves. Historic release notes/game captures keep their dated identity; old releases remain immutable.

Verification: native DE/EN menus at compact/720p/1080p/4K, unchanged UI/controller/session tests, installer boundary tests, native Linux/Windows package checks and an actual Windows Alpha-7-to-8 upgrade preserving an existing profile. Capture new production empty menu and update current screenshot provenance. Qt surface skips HTML/CSS detector. Keep other design-system drift outside scope; only the authorized product name/title changes globally.
