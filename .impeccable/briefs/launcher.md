# Launcher visual refinement — user-selected retro direction

User request: make the existing launcher modern and attractive. Confirmed preference: “Stärker im Pokémon-/Retro-Stil”. Preserve the existing native Qt desktop application and its workflows; no website or new application. German language, two-player couch co-op, Linux/Zorin/Bazzite and Windows. Safeguards for saves, assignment, session/ROM mismatch and shutdown remain mandatory.

Mode: Operate. First viewport at 1280×720 shows product title, actual selected ROM / empty state, controller assignment / empty state, one primary start action, all eight setup/management actions and real session status. Narrow windows stack the two working areas and keep focus-scrolling. 4K scales controls/text for couch use.

Implementation directly in native widgets, no generated image comp and no stored global build preference. The visual system refines the actual interface into a restrained handheld-game vocabulary: graphite/teal casing, warm ivory game information, coral-red start, gold focus, embedded open-license pixel lettering for headings, native system font for body/control text. Icons are our own simple native geometry. No Nintendo artwork, ROM imagery, sprites, logo/font rips, fake gameplay or false ready/connection indicators.

Signature interaction: controller/keyboard focus is a crisp gold selection outline, consistent for every action. No decorative launch animation, no delay before menus can be used. Player chrome uses coral/cyan identifiers. Existing controller ownership and input code remain unchanged.

Required screenshots: empty and prepared main menu at 1280×720; narrow 640×360 (first screen plus focused exit); 1920×1080 and 3840×2160; existing DS player layouts / wrapped help; one normal settings dialog. Native Qt screenshots, not HTML. No web detector applies to this C++/Qt surface. Inspect in at most two batched capture rounds, with any fixes grouped between.

Verification: native CTest UI/controller/focus/session regressions and full Python suite; source-font embedding checked by native UI test; labels/icons must fit. Before binary distribution require native Linux/Windows builds with full matching source. Actual Pokémon/physical-device/Bazzite testing is not implied by styling checks.
