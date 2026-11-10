# Help test CouchSplit Alpha 12

**English** · [Deutsch](de/testing.md) · [Project](../README.md)

Two players, one computer, one shared screen. We are looking for Windows and Bazzite testers for experimental English/German FireRed co-op. A short report is useful even if installation fails. No full playthrough is required.

## Download and requirements

[Download Alpha 12](https://github.com/Philmenting/couchsplit/releases/tag/v0.2.0-alpha.12): Windows installer or portable ZIP, Bazzite installer, and Zorin OS 18 / Ubuntu 24.04 package.

Bring two controllers and your own supported unmodified English FireRed 1.0/1.1 or German original. The launcher checks the exact file fingerprint; other revisions, games and arbitrary ROM hacks do not gain shared-world co-op. No ROM or save downloads are provided. [Exact compatibility and limits](firered.md).

Finish any running session before updating. Use separate test profiles or back up both saves before testing experimental gameplay. Choose English or German in the launcher, select the ROM, assign the controllers and enable **Settings → FireRed co-op (experimental)**.

## A 15-minute first test

If starting a new game, introductory progression may take longer. Report how far you get; reaching a battle is optional for an installation report.

1. Install and start. Tell us whether both views appear and whether setup was clear.
2. Check that each controller moves only its assigned player. Once both players can move freely, meet on the same map and check the partner display.
3. If both players are ready for encounters, enter a shared battle. Choose an action for each player and try an available own Pokémon switch.
4. Save normally in **both** games, wait for saving to finish, then end the session. Restart and check both teams, positions and money.
5. Optionally check sound, fullscreen and controller reconnection. Tell us about stutter or uncomfortable fan noise.

If something fails, stop at that step and describe it. A successful test is worth reporting too.

## Send a report

[Open the short test-report form](https://github.com/Philmenting/couchsplit/issues/new?template=playtest.yml). English and German reports are welcome. Without a GitHub account, reply to the Reddit post where you found this project.

Include version, operating system, controller models, completed steps and what happened. Do not upload ROMs, saves, account credentials or full personal profiles. For logs, share only the relevant excerpt after removing personal paths or identifiers.

We aim to collect five useful first reports, including Windows and Bazzite. This is a target, not an existing tester count. There is no payment or guaranteed support response time.

## What is verified?

Alpha 12 has automated Windows/Linux build and package checks and targeted Linux original-ROM scenarios. Those used prepared checkpoints and virtual controllers. Real Bazzite/controller gameplay, a complete natural campaign and multi-hour sessions remain unverified. Enigma Berries and other documented special cases have limitations. [Evidence and known limits](development-status.md).
