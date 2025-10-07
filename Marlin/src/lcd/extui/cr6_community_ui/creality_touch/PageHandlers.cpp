#include "../../../../inc/MarlinConfigPre.h"

#if ENABLED(DGUS_LCD_UI_CR6_COMM)

#include "../DGUSDisplayDef.h"
#include "../DGUSDisplay.h"
#include "../DGUSScreenHandler.h"

#include "../../../../module/temperature.h"
#include "../../../../module/motion.h"
#include "../../../../module/planner.h"
#include "../../../../feature/pause.h"
#include "../../../../gcode/gcode.h"

#if ENABLED(FILAMENT_RUNOUT_SENSOR)
#include "../../../../feature/runout.h"
#endif

#include "../../../../module/settings.h"

#include "../../ui_api.h"
#include "../../../marlinui.h"

#include "PageHandlers.h"


// Definitions of page handlers

void MainMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_MAINENTERKEY:
            switch (buttonValue) {
                case 1:
                    // Try to mount an unmounted card (BTT SKR board has especially some trouble sometimes)
                    card.mount();
                    ScreenHandler.SDCardInserted();
                    break;

                case 2:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_PREPARE);
                    break;

                case 3:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_SETUP);
                    break;

                case 4:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_CALIBRATE);
                    break;
            }
            break;
    }
}

void SetupMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_PREPAREENTERKEY:
            switch(buttonValue) {
                case 5: // About
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_INFO);
                    break;

                case 7: // Reset to factory settings
                    settings.reset();
                    settings.save();

                    ExtUI::injectCommands_P(PSTR("M300"));

                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MAIN, false);
                    ScreenHandler.setstatusmessagePGM(PSTR("Restored default settings. Please turn your printer off and then on to complete the reset"));
                    break;
            }
            break;

        case VP_BUTTON_TEMPCONTROL:
            if (buttonValue == 2) ScreenHandler.GotoScreen(DGUSLCD_SCREEN_TEMP);
            break;

        case VP_BUTTON_ADJUSTENTERKEY:
            ScreenHandler.HandleLEDToggle();
            break;
    }
}

void LevelingModeHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_BEDLEVELKEY:
            switch (buttonValue) {
                case 1:
                    queue.enqueue_one_P("G28 U0");
                    queue.enqueue_one_P("G0 Z0");
                break;

                case 2:
                    // Increase Z-offset
                    ExtUI::smartAdjustAxis_steps(ExtUI::mmToWholeSteps(0.01, ExtUI::axis_t::Z), ExtUI::axis_t::Z, true);;
                    ScreenHandler.ForceCompleteUpdate();
                    ScreenHandler.RequestSaveSettings();
                    break;

                case 3:
                    // Decrease Z-offset
                    ExtUI::smartAdjustAxis_steps(ExtUI::mmToWholeSteps(-0.01, ExtUI::axis_t::Z), ExtUI::axis_t::Z, true);;
                    ScreenHandler.ForceCompleteUpdate();
                    ScreenHandler.RequestSaveSettings();
                    break;
            }

            break;

        case VP_BUTTON_PREPAREENTERKEY:
            if (buttonValue == 9) {
                #if DISABLED(HOTEND_IDLE_TIMEOUT)
                    thermalManager.disable_all_heaters();
                #endif

                ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MAIN, false);
            }
#if HAS_MESH
            if (buttonValue == 1) {
                // TODO: set state for "view leveling mesh"
                ScreenHandler.SetViewMeshLevelState();
                ScreenHandler.InitMeshValues();

                ScreenHandler.GotoScreen(DGUSLCD_SCREEN_LEVELING);
            }
#endif
            break;

        case VP_BUTTON_MAINENTERKEY:
            // Go to leveling screen
            ExtUI::injectCommands_P("G28 U0\nG29 U0");
#if HAS_MESH
            ScreenHandler.ResetMeshValues();
#endif
            dgusdisplay.WriteVariable(VP_MESH_SCREEN_MESSAGE_ICON, static_cast<uint16_t>(MESH_SCREEN_MESSAGE_ICON_LEVELING));
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_LEVELING);
            break;
    }
}

void LevelingHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_BEDLEVELKEY:
            if (!ScreenHandler.HasCurrentSynchronousOperation()) {
                ScreenHandler.PopToOldScreen();
            } else {
                ScreenHandler.setstatusmessagePGM("Wait for leveling completion...");
            }

            break;
    }
}

void TempMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_ADJUSTENTERKEY:
            switch (buttonValue) {
                case 3:
                    ScreenHandler.HandleFanToggle();
                break;
            }

            break;

        case VP_BUTTON_TEMPCONTROL:
            switch (buttonValue){ 
                case 3:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_TEMP_PLA);
                    break;

                case 4:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_TEMP_ABS);
                    break;
            }
            break;
    }
}

void PrepareMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_PREPAREENTERKEY:
            switch (buttonValue){
                case 3:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MOVE10MM);
                    break;

                case 6:
                    // Disable steppers
                    ScreenHandler.HandleMotorLockUnlock(var, &buttonValue);
                    break;
            }
        break;

        case VP_BUTTON_HEATLOADSTARTKEY: 
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_FEED);
        break;

        case VP_BUTTON_COOLDOWN:
            ScreenHandler.HandleAllHeatersOff(var, &buttonValue);
            break;

        case VP_BUTTON_TEMPCONTROL:
            switch (buttonValue) {
                case 5:
#if HAS_PREHEAT
                    thermalManager.setTargetHotend(ExtUI::getMaterial_preset_E(0), 0);
                    thermalManager.setTargetBed(ExtUI::getMaterial_preset_B(0));
#else
                    thermalManager.setTargetHotend(ExtUI::getTargetTemp_celsius(ExtUI::E0), 0);
#if HAS_HEATED_BED
                    thermalManager.setTargetBed(ExtUI::getTargetTemp_celsius(ExtUI::heater_t::BED));
#endif
#endif

                    break;

                case 6:
                    // Some Marlin configs only expose a single preheat preset. Guard access to the second preset.
#if PREHEAT_COUNT > 1
#if HAS_PREHEAT
                    thermalManager.setTargetHotend(ExtUI::getMaterial_preset_E(1), 0);
                    thermalManager.setTargetBed(ExtUI::getMaterial_preset_B(1));
#else
                    thermalManager.setTargetHotend(ExtUI::getTargetTemp_celsius(ExtUI::E0), 0);
#if HAS_HEATED_BED
                    thermalManager.setTargetBed(ExtUI::getTargetTemp_celsius(ExtUI::heater_t::BED));
#endif
#endif
#else
                    // Fallback to preset 0 if preset 1 is unavailable
#if HAS_PREHEAT
                    thermalManager.setTargetHotend(ExtUI::getMaterial_preset_E(0), 0);
                    thermalManager.setTargetBed(ExtUI::getMaterial_preset_B(0));
#else
                    thermalManager.setTargetHotend(ExtUI::getTargetTemp_celsius(ExtUI::E0), 0);
#if HAS_HEATED_BED
                    thermalManager.setTargetBed(ExtUI::getTargetTemp_celsius(ExtUI::heater_t::BED));
#endif
#endif
#endif
                    break;
            }
            break;
    }

    ScreenHandler.ForceCompleteUpdate();
}

void TuneMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_ADJUSTENTERKEY:
            switch (buttonValue) {
                case 2:
                    ScreenHandler.GotoScreen(ExtUI::isPrintingPaused() ? DGUSLCD_SCREEN_PRINT_PAUSED : DGUSLCD_SCREEN_PRINT_RUNNING, false);
                    break;

                case 3:
                    ScreenHandler.HandleFanToggle();
                break;

                case 4:
                    ScreenHandler.HandleLEDToggle();
                break;
            }
    }
}

void PrintRunningMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_ADJUSTENTERKEY:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_TUNING);
        break;

        case VP_BUTTON_PAUSEPRINTKEY:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_DIALOG_PAUSE);
        break;

        case VP_BUTTON_STOPPRINTKEY:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_DIALOG_STOP);
        break;
    }
}

void PrintPausedMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP) {
        case VP_BUTTON_RESUMEPRINTKEY:
#if ENABLED(FILAMENT_RUNOUT_SENSOR)
            runout.reset();
#endif

            if (!ScreenHandler.HandlePendingUserConfirmation()) {
                ExtUI::resumePrint();
                ScreenHandler.GotoScreen(DGUSLCD_SCREEN_PRINT_RUNNING);
            }
            break;

        case VP_BUTTON_ADJUSTENTERKEY:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_TUNING);
        break;

        case VP_BUTTON_STOPPRINTKEY:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_DIALOG_STOP);
        break;
    }
}

void PrintPauseDialogHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP){
        case VP_BUTTON_PAUSEPRINTKEY:
            switch (buttonValue) {
                case 2:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_PRINT_RUNNING);
                    ScreenHandler.setstatusmessagePGM(PSTR("Pausing print - please wait..."));
                    ExtUI::pausePrint();
                    break;

                case 3:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_PRINT_RUNNING);
                    break;
            }
            break;
    }
}

void PrintFinishMenuHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP){
        case VP_BUTTON_MAINENTERKEY:
            switch (buttonValue) {
                case 5:
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MAIN);
                    break;
            }
            break;
    }
}

void FilamentRunoutHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP){
        case VP_BUTTON_RESUMEPRINTKEY:
            ExtUI::resumePrint();
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_PRINT_RUNNING);
        break;

        case VP_BUTTON_STOPPRINTKEY:
            ExtUI::stopPrint();
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MAIN);
        break;
    }
}

void StopConfirmScreenHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP){
        case VP_BUTTON_STOPPRINTKEY:
            switch (buttonValue) {
                case 2:
                    ExtUI::stopPrint();
                    ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MAIN);
                break;

                case 3:
                    ScreenHandler.GotoScreen(ExtUI::isPrintingPaused() ? DGUSLCD_SCREEN_PRINT_PAUSED : DGUSLCD_SCREEN_PRINT_RUNNING);
                break;
            }
        break;
    }
}

void PreheatSettingsScreenHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    switch (var.VP){
        case VP_BUTTON_PREPAREENTERKEY:
            // Save button, save settings and go back
            ScreenHandler.RequestSaveSettings();
            ScreenHandler.PopToOldScreen();
        break;

        case VP_BUTTON_COOLDOWN: // You can't make this up
            // Back button, discard settings
            settings.load();
            ScreenHandler.PopToOldScreen();
            break;
    }
}

void MoveHandler(DGUS_VP_Variable &var, unsigned short buttonValue) {
    if (var.VP == VP_BUTTON_MOVEKEY) {
        switch (buttonValue) {
        case 1:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MOVE10MM, false);
            break;
        case 2:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MOVE1MM, false);
            break;
        case 3:
            ScreenHandler.GotoScreen(DGUSLCD_SCREEN_MOVE01MM, false);
            break;
        case 4:
            // The original code temporarily modified probe.settings to disable
            // preheating for homing. Marlin no longer exposes probe.settings,
            // so skip that behavior and just execute the home command.
            ExtUI::injectCommands_P("G28");
            break;
        }
    }
}

// Register the page handlers
#define PAGE_HANDLER(SCRID, HDLRPTR) { .ScreenID=SCRID, .Handler=HDLRPTR },
const struct PageHandler PageHandlers[] PROGMEM = {
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_MAIN, MainMenuHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_SETUP, SetupMenuHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_ZOFFSET_LEVEL, LevelingModeHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_LEVELING, LevelingHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_TEMP, TempMenuHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_TEMP_PLA, PreheatSettingsScreenHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_TEMP_ABS, PreheatSettingsScreenHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_TUNING, TuneMenuHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_MOVE01MM, MoveHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_MOVE1MM, MoveHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_MOVE10MM, MoveHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_FILAMENTRUNOUT1, FilamentRunoutHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_FILAMENTRUNOUT2, FilamentRunoutHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_DIALOG_STOP, StopConfirmScreenHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_PRINT_RUNNING, PrintRunningMenuHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_PRINT_PAUSED, PrintPausedMenuHandler)
    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_PRINT_FINISH, PrintFinishMenuHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_DIALOG_PAUSE, PrintPauseDialogHandler)

    PAGE_HANDLER(DGUSLCD_Screens::DGUSLCD_SCREEN_PREPARE, PrepareMenuHandler)

    // Terminating
    PAGE_HANDLER(static_cast<DGUSLCD_Screens>(0) ,0)
};

void DGUSCrealityDisplay_HandleReturnKeyEvent(DGUS_VP_Variable &var, void *val_ptr) {
  const struct PageHandler *map = PageHandlers;
  const uint16_t *ret;
  const DGUSLCD_Screens current_screen = DGUSScreenHandler::getCurrentScreen();

  while ((ret = (uint16_t*) pgm_read_ptr(&(map->Handler)))) {
    if ((map->ScreenID) == current_screen) {
        uint16_t button_value = uInt16Value(val_ptr);
        
        SERIAL_ECHOPAIR("Invoking handler for screen ", current_screen);
        SERIAL_ECHOLNPAIR("with VP=", var.VP, " value=", button_value);

        map->Handler(var, button_value);
        return;
    }

    map++;
  }
}

#endif
