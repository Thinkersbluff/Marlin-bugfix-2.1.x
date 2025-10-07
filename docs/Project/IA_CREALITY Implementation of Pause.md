Good — below I extracted and explained just the Pause-related pieces and how ia_creality ties them to Marlin’s pause/confirmation API. I list the key functions, where they live, what they do, and the message/response flow end‑to‑end so you can reuse the design or adapt it to CR6-SE.

Summary of the flow (high level)

Marlin triggers a pause/confirmation via the ExtUI API (MarlinUI calls into the ExtUI layer).
The shared ExtUI API maps Marlin’s PauseMessage enum to a textual prompt via stdOnPauseMode() (in ui_api.cpp) and calls onUserConfirmRequired(...).
The ia_creality ExtUI implements onUserConfirmRequired(...) to:
show the UI pause screen (page +78 for this image),
present a localized message,
and wait for user input.
When the touchscreen user presses buttons (Yes/No/Stop/Resume on the DWIN panel), the RTS layer (ia_creality_rts.cpp) receives those events and converts them to calls to ExtUI::pausePrint(), ExtUI::resumePrint(), ExtUI::stopPrint() or to set the pause_menu_response.
ExtUI::pausePrint(), resumePrint(), stopPrint() call Marlin UI functions (ui.pause_print(), ui.resume_print(), ui.abort_print()), completing the loop.
Where to look (files & concrete behavior)

Logical mapping of PauseMessage → request for user confirmation
File: ui_api.cpp
Function: stdOnPauseMode(...)
What it does:
Receives Marlin pause events (PauseMessage + PauseMode).
Sets pauseModeStatus = message;
Calls onUserConfirmRequired(...) with the appropriate translated message for each PauseMessage (parking, changing, unload, waiting, insert, load, purge, resume, heat, etc.).
Why it matters:
This is the canonical place where core Marlin pause logic asks the UI to request confirmation or show a prompt. Any ExtUI must implement onUserConfirmRequired(...) to display/respond.
Snippet (behavioral outline)

stdOnPauseMode maps enum cases:
PAUSE_MESSAGE_PARKING → onUserConfirmRequired(MSG_PAUSE_PRINT_PARKING)
PAUSE_MESSAGE_CHANGING → onUserConfirmRequired(MSG_FILAMENT_CHANGE_INIT)
PAUSE_MESSAGE_RESUME → onUserConfirmRequired(MSG_FILAMENT_CHANGE_RESUME)
etc.
ExtUI entry that shows a pause UI and status text (ia_creality)
File: ia_creality_extui.cpp
Functions of interest:
void onUserConfirmRequired(const char *const msg) — main entry point used by stdOnPauseMode() and other callers.
void onPauseMode(...) (thin wrapper that calls stdOnPauseMode if ADVANCED_PAUSE_FEATURE is enabled).
void onPrintTimerPaused() / onPrintTimerStarted() / onPrintTimerStopped() — show pause screens/status too.
What onUserConfirmRequired does in ia_creality:
Sets printerStatusKey[1] = 4; tpShowStatus = false; (internal UI state flags).
Guards against duplicate messages using lastPauseMsgState.
Switches to the DWIN pause screen: rts.sendData(ExchangePageBase + 78, ExchangepageAddr); (page 78 in this specific CR10 image).
Sets a short textual status message with onStatusChanged(...) (e.g., "Press Yes to Continue", "Load Filament to Continue", "Add Filament and Press Yes to Reheat", etc).
For purge/option cases it composes a "Yes to Continue / No to Purge" style message.
For PAUSE_MESSAGE_HEATING it switches to a different page (ExchangePageBase + 68) and displays "Reheating".
For the default/status case, it calls:
setPauseMenuResponse(PAUSE_RESPONSE_RESUME_PRINT);
setUserConfirmed();
i.e., default direction is to auto-confirm resume if nothing else required.
Why it matters:
This is the concrete implementation that shows the UI and instructs the user what to do (buttons & prompt). If you adapt for CR6-SE, you’ll replicate the same mapping but use the CR6 page IDs & assets.
How the UI sends the user's choice back (RTS -> firmware)
File: ia_creality_rts.cpp
Where: Event/telegram handling switch (around the Checkkey / case Printfile / case PrintChoice logic; see ~lines 680–770).
What it does:
The touchscreen posts button events (addresses named Pauseprint, Resumeprint, Stopprint, etc).
RTS decodes those and:
If Resumeprint with data==1: calls resumePrint() (this calls ExtUI::resumePrint()) and then returns to a status page.
If Pauseprint data==1: switches to a pause page (for example ExchangePageBase + 54) and calls pausePrint() where appropriate.
If stop is confirmed, calls stopPrint() and navigates UI accordingly.
For confirm dialogs (Yes/No), RTS may set pause_resume_selected or directly call the corresponding ExtUI helper that in turn calls Marlin UI.
Why it matters:
This shows how the touchscreen drives Marlin by invoking the ExtUI methods (pause/resume/stop) when the user taps the on-panel buttons.
Example handling (behavioral outline)

When the DWIN panel's "Resume" button is pressed, RTS receives a telegram:
RTS code path: when recdat.addr == Resumeprint and recdat.data[0] == 1 -> resumePrint(); printerStatusKey[1] = 0; show_status = true; sendData(ExchangePageBase + 53, ExchangepageAddr);
resumePrint() is ExtUI::resumePrint() which calls ui.resume_print() (Marlin core resume logic).
When the DWIN panel's "Pause" button is pressed: similar chain that calls pausePrint() -> ui.pause_print().
Small helpers and state in the shared UI API
File: ui_api.cpp
Symbols/methods:
void setUserConfirmed() — clears wait_for_user or sets confirmation state used by some pause flows.
void setPauseMenuResponse(PauseMenuResponse) — sets pause_menu_response variable used by Advanced Pause flow (e.g., "Purge more" menu choice).
bool awaitingUserConfirm() — returns whether UI is waiting for user confirm (wait_for_user) or if host keepalive pause used.
void pausePrint() / resumePrint() / stopPrint() — proxy functions calling ui.pause_print() / ui.resume_print() / ui.abort_print() so ExtUI implementations simply call these to trigger Marlin pause actions.
PauseMessage pauseModeStatus — current pause message stored by stdOnPauseMode.
Why it matters:
The shared API provides the glue: ExtUI implements onUserConfirmRequired (UI display & prompt) while the shared ui_api holds the canonical pause state and helpers for responding to user actions.
Key design points to reuse

Keep stdOnPauseMode as the canonical mapping from Marlin pause enum → user prompt. ExtUI implementations should just implement onUserConfirmRequired(...) to show UI and return user input.
Two roles:
Shared role (ui_api): maintain pause state, hold pause_menu_response and wait_for_user, provide pausePrint()/resumePrint()/stopPrint() that call the Marlin UI functions.
ExtUI role (ia_creality): translate onUserConfirmRequired(...) into a screen and buttons, then translate user button presses back into ExtUI calls (or set pause_menu_response) via the RTS handler.
For compatibility across screensets simply:
Keep the stdOnPauseMode → onUserConfirmRequired() semantics unchanged.
Re-implement onUserConfirmRequired() in the CR6-SE extui to show your CR6 screens (instead of CR10 pages) and wire the CR6 button event addresses to the same response actions (call setPauseMenuResponse(), setUserConfirmed(), and call ExtUI::pausePrint()/resumePrint()/stopPrint() as appropriate).
Concrete pointers for porting to CR6-SE

Implement the CR6-SE equivalents of:
onUserConfirmRequired(const char*) — show the pause dialog on the CR6 page and set the message text.
RTS receive-handler — map CR6 button telegrams -> calls to ExtUI::setPauseMenuResponse(...), ExtUI::setUserConfirmed(), ExtUI::pausePrint(), ExtUI::resumePrint(), ExtUI::stopPrint() depending on which button was pressed.
Keep stdOnPauseMode usage exactly as-is — no change necessary there.
If you want, next steps I can take for you

Extract & paste the exact RTS telegram addresses and the small switch/case handler lines that map the panel buttons to these pause actions (so you can map CR6 event ids to the same calls). I have already located the relevant switch in ia_creality_rts.cpp (~lines 680–770) and can produce the relevant minimal snippets.
Create a short reference table: DWIN variable/address names used by ia_creality (Pauseprint, Resumeprint, Stopprint, etc.) and the corresponding calls or responses (pause/resume/stop/setPauseMenuResponse).
Create a minimal CR6-SE template of onUserConfirmRequired(...) + RTS handler skeleton so you can plug your CR6 page numbers and button addresses into it.
Which would be most useful right now:

(A) A compact table mapping ia_creality DWIN variable names → Marlin action (Pause/Resume/Stop/Confirm)?
(B) A focused patch skeleton implementing the same behavior for CR6-SE (you supply CR6 page & button addresses), or
(C) Paste the exact RTS code snippets that handle Resume/Pause/Stop so you can copy them into your CR6 handler?
Pick one and I’ll produce the concrete mapping/snippets.