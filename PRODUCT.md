# Product

<!-- impeccable:product-schema 1 -->

## Platform

Native desktop application: Qt 6 on Linux and Windows. Bazzite runs the Linux application in its existing supported runtime. This is not a mobile app or a web page.

## Users

Two players sharing one screen and two controllers. The launcher offers English and German. Simple couch use comes first; developer tools remain secondary.

## Product Purpose

Start two separate Project PM emulator processes in one local co-op window, select a prepared user-supplied ROM, assign controllers, configure the screen arrangement, and preserve independent saves.

## Operating Context

Existing Qt/SDL launcher, desktop setup, later controller navigation. Windows, Zorin OS 18 and Bazzite are the supported package targets. Existing safeguards and AGENTS.md govern session ownership, input isolation and saves.

## Capabilities and Constraints

ROM selection/import, explicit controller assignment, button mapping, screen layout, save backups/restoration, emulator/firmware settings, session logs and orderly shutdown already exist. No commercial ROM, sprites, BIOS, firmware or personal saves may be committed. Maintain all working safeguards during visual edits.

## Brand Commitments

The user requests a modern, attractive launcher and explicitly selected: “Stärker im Pokémon-/Retro-Stil”. The user subsequently requested the name to focus on couch co-op and split screen rather than Platinum. The project is now CouchSplit; the interface offers English and German. Preserve the established retro identity. No global build-workflow preference was supplied.

## Evidence on Hand

Working native source and UI/controller tests; previous Alpha-4 platform builds; actual two-player Pokémon gameplay tests documented separately. Visual test captures use native Qt widgets, actual selected user-ROM paths and virtual controllers. Physical-device/native-platform gameplay is not implied by a UI screenshot.

## Product Principles

- Starting two players is the primary action.
- Controller and keyboard focus must remain obvious.
- Save preservation and honest session state outweigh decoration.
- Readable settings and responsive layouts take priority over retro imitation.
