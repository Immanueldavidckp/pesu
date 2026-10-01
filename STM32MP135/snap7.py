import snap7
from snap7.util import get_int, get_real, get_dint

# PLC Connection Parameters
PLC_IP = "192.168.0.1"
RACK = 0
SLOT = 1


def get_plc_data():
    client = snap7.client.Client()

    try:
        client.connect(PLC_IP, RACK, SLOT)
        if not client.get_connected():
            print(f"Failed to connect to PLC at {PLC_IP}")
            return

        print(f"Connected to {PLC_IP}. Fetching data...")

        # --- READ DB2 (Configurations) ---
        # Size per datasheet: offset 56 (Real, 4 bytes) + 4 = 60 bytes
        db2_raw = client.db_read(2, 0, 60)
        db2_data = {
            "MixingRatio_Height_RawMatTk1":  (get_int(db2_raw, 0),  "mm"),
            "MixingRatio_Height_RawMatTk2":  (get_int(db2_raw, 2),  "mm"),
            "MixingRatio_Height_RawMatTk3":  (get_int(db2_raw, 4),  "mm"),
            "MixingRatio_Height_RawMatTk4":  (get_int(db2_raw, 6),  "mm"),
            "MixingRatio_Height_HotWaterTk": (get_int(db2_raw, 8),  "mm"),
            "MixingRatio_Perc_RawMatTk1":    (get_int(db2_raw, 10), "%"),
            "MixingRatio_Perc_RawMatTk2":    (get_int(db2_raw, 12), "%"),
            "MixingRatio_Perc_RawMatTk3":    (get_int(db2_raw, 14), "%"),
            "MixingRatio_Perc_RawMatTk4":    (get_int(db2_raw, 16), "%"),
            "MixingRatio_Perc_HotWaterTk":   (get_int(db2_raw, 18), "%"),
            "DelayBetweenSequence":          (get_dint(db2_raw, 20), "ms"),
            "MixingTk_Agitator_On":          (get_dint(db2_raw, 24), "ms"),
            "HotReacDeepTk_Agitator_On":     (get_dint(db2_raw, 28), "ms"),
            "ColdReacDeepTk_Agitator_On":    (get_dint(db2_raw, 32), "ms"),
            "MixingTk_Pump_On":              (get_dint(db2_raw, 36), "ms"),
            "HotWaterTk_DirectPump_On":      (get_dint(db2_raw, 40), "ms"),
            "HotWaterTk_CondPump_On":        (get_dint(db2_raw, 44), "ms"),
            "ReturnLine_Pump_On":            (get_dint(db2_raw, 48), "ms"),
            "GeyserSetPoint_Temp":           (get_real(db2_raw, 52), "C"),
            "GeyserSetPoint_Differential":   (get_real(db2_raw, 56), "C"),
        }

        # --- READ DB3 (Full Data) ---
        # Size per datasheet: last field (Elapsed_HotWaterTk_ReturnPump_On)
        # is a DInt at offset 320 -> 320 + 4 = 324 bytes
        db3_raw = client.db_read(3, 0, 324)
        db3_data = {
            # Current Levels
            "Lvl_Height_RawMatTk1":  (get_int(db3_raw, 0),  "mm"),
            "Lvl_Height_RawMatTk2":  (get_int(db3_raw, 2),  "mm"),
            "Lvl_Height_RawMatTk3":  (get_int(db3_raw, 4),  "mm"),
            "Lvl_Height_RawMatTk4":  (get_int(db3_raw, 6),  "mm"),
            "Lvl_Height_HotwaterTk": (get_int(db3_raw, 8),  "mm"),
            "Lvl_Perc_RawMatTk1":    (get_int(db3_raw, 10), "%"),
            "Lvl_Perc_RawMatTk2":    (get_int(db3_raw, 12), "%"),
            "Lvl_Perc_RawMatTk3":    (get_int(db3_raw, 14), "%"),
            "Lvl_Perc_RawMatTk4":    (get_int(db3_raw, 16), "%"),
            "Lvl_Perc_HotwaterTk":   (get_int(db3_raw, 18), "%"),

            # Pressures
            "Press_MixingTk":     (get_real(db3_raw, 20), "bar"),
            "Press_ReacDeepTk1":  (get_real(db3_raw, 24), "bar"),
            "Press_ReacDeepTk2":  (get_real(db3_raw, 28), "bar"),
            "Press_ReturnLine":   (get_real(db3_raw, 32), "bar"),

            # Temperatures
            "Temp_PHE_In":            (get_real(db3_raw, 36), "C"),
            "Temp_PHE_Out":           (get_real(db3_raw, 40), "C"),
            "Temp_ShellTube_In":      (get_real(db3_raw, 44), "C"),
            "Temp_ShellTube_Out":     (get_real(db3_raw, 48), "C"),
            "Temp_HotReacDeepTk_In":  (get_real(db3_raw, 52), "C"),
            "Temp_Geyser_Out":        (get_real(db3_raw, 56), "C"),

            # Previous Levels (Height)
            "Prev_Height_RawMatTk1":   (get_int(db3_raw, 60), "mm"),
            "Prev_Height_RawMatTk2":   (get_int(db3_raw, 62), "mm"),
            "Prev_Height_RawMatTk3":   (get_int(db3_raw, 64), "mm"),
            "Prev_Height_RawMatTk4":   (get_int(db3_raw, 66), "mm"),
            "Prev_Height_HotwaterTk":  (get_int(db3_raw, 68), "mm"),

            # Previous Levels (Percentage)
            # NOTE: datasheet lists Tk3=68 and Tk4=72 (duplicate offsets,
            # almost certainly a typo). Continuing the sequence at 74/76/78
            # until confirmed with the PLC programmer.
            "Prev_Perc_RawMatTk1":    (get_int(db3_raw, 70), "%"),
            "Prev_Perc_RawMatTk2":    (get_int(db3_raw, 72), "%"),
            "Prev_Perc_RawMatTk3":    (get_int(db3_raw, 74), "%"),   # UNCONFIRMED - see note
            "Prev_Perc_RawMatTk4":    (get_int(db3_raw, 76), "%"),   # UNCONFIRMED - see note
            "Prev_Perc_HotwaterTk":   (get_int(db3_raw, 78), "%"),   # UNCONFIRMED - see note

            # Main Energy Meter
            "Main_Energy": (get_real(db3_raw, 80),  "Wh"),
            "Main_V_R":    (get_real(db3_raw, 84),  "V"),
            "Main_V_Y":    (get_real(db3_raw, 88),  "V"),
            "Main_V_B":    (get_real(db3_raw, 92),  "V"),
            "Main_I_R":    (get_real(db3_raw, 96),  "A"),
            "Main_I_Y":    (get_real(db3_raw, 100), "A"),
            "Main_I_B":    (get_real(db3_raw, 104), "A"),
            "Main_PF_R":   (get_real(db3_raw, 108), "-"),
            "Main_PF_Y":   (get_real(db3_raw, 112), "-"),
            "Main_PF_B":   (get_real(db3_raw, 116), "-"),
            "Main_P_R":    (get_real(db3_raw, 120), "W"),
            "Main_P_Y":    (get_real(db3_raw, 124), "W"),
            "Main_P_B":    (get_real(db3_raw, 128), "W"),

            # Stage 1 Energy Meter
            "S1_Energy": (get_real(db3_raw, 132), "Wh"),
            "S1_V_R":    (get_real(db3_raw, 136), "V"),
            "S1_V_Y":    (get_real(db3_raw, 140), "V"),
            "S1_V_B":    (get_real(db3_raw, 144), "V"),
            "S1_I_R":    (get_real(db3_raw, 148), "A"),
            "S1_I_Y":    (get_real(db3_raw, 152), "A"),
            "S1_I_B":    (get_real(db3_raw, 156), "A"),
            "S1_PF_R":   (get_real(db3_raw, 160), "-"),
            "S1_PF_Y":   (get_real(db3_raw, 164), "-"),
            "S1_PF_B":   (get_real(db3_raw, 168), "-"),
            "S1_P_R":    (get_real(db3_raw, 172), "W"),
            "S1_P_Y":    (get_real(db3_raw, 176), "W"),
            "S1_P_B":    (get_real(db3_raw, 180), "W"),

            # Stage 2 Energy Meter
            "S2_Energy": (get_real(db3_raw, 184), "Wh"),
            "S2_V_R":    (get_real(db3_raw, 188), "V"),
            "S2_V_Y":    (get_real(db3_raw, 192), "V"),
            "S2_V_B":    (get_real(db3_raw, 196), "V"),
            "S2_I_R":    (get_real(db3_raw, 200), "A"),
            "S2_I_Y":    (get_real(db3_raw, 204), "A"),
            "S2_I_B":    (get_real(db3_raw, 208), "A"),
            "S2_PF_R":   (get_real(db3_raw, 212), "-"),
            "S2_PF_Y":   (get_real(db3_raw, 216), "-"),
            "S2_PF_B":   (get_real(db3_raw, 220), "-"),
            "S2_P_R":    (get_real(db3_raw, 224), "W"),
            "S2_P_Y":    (get_real(db3_raw, 228), "W"),
            "S2_P_B":    (get_real(db3_raw, 232), "W"),

            # Config Snapshot (echoed from DB2, read-only verification)
            "Snap_MixHeight_RawMatTk1":  (get_int(db3_raw, 236), "mm"),
            "Snap_MixHeight_RawMatTk2":  (get_int(db3_raw, 238), "mm"),
            "Snap_MixHeight_RawMatTk3":  (get_int(db3_raw, 240), "mm"),
            "Snap_MixHeight_RawMatTk4":  (get_int(db3_raw, 242), "mm"),
            "Snap_MixHeight_HotwaterTk": (get_int(db3_raw, 244), "mm"),
            "Snap_MixPerc_RawMatTk1":    (get_int(db3_raw, 246), "%"),
            "Snap_MixPerc_RawMatTk2":    (get_int(db3_raw, 248), "%"),
            "Snap_MixPerc_RawMatTk3":    (get_int(db3_raw, 250), "%"),
            "Snap_MixPerc_RawMatTk4":    (get_int(db3_raw, 252), "%"),
            "Snap_MixPerc_HotwaterTk":   (get_int(db3_raw, 254), "%"),
            "Snap_DelayBetweenSequence":         (get_dint(db3_raw, 256), "ms"),
            "Snap_MixingTk_Agitator_On":         (get_dint(db3_raw, 260), "ms"),
            "Snap_HotReacDeepTk_Agitator_On":    (get_dint(db3_raw, 264), "ms"),
            "Snap_ColdReacDeepTk_Agitator_On":   (get_dint(db3_raw, 268), "ms"),
            "Snap_MixingTk_Pump_On":             (get_dint(db3_raw, 272), "ms"),
            "Snap_HotWaterTk_DirectPump_On":     (get_dint(db3_raw, 276), "ms"),
            "Snap_HotWaterTk_CondPump_On":       (get_dint(db3_raw, 280), "ms"),
            "Snap_ReturnLine_Pump_On":           (get_dint(db3_raw, 284), "ms"),

            # Elapsed Times
            "Elapsed_DelayBetweenSequence":         (get_dint(db3_raw, 288), "ms"),
            "Elapsed_MixingTk_Agitator_On":         (get_dint(db3_raw, 292), "ms"),
            "Elapsed_HotReacDeepTk_Agitator_On":    (get_dint(db3_raw, 296), "ms"),
            "Elapsed_ColdReacDeepTk_Agitator_On":   (get_dint(db3_raw, 300), "ms"),
            "Elapsed_MixingTk_Pump_On":             (get_dint(db3_raw, 304), "ms"),
            "Elapsed_HotWaterTk_DirectPump_On":     (get_dint(db3_raw, 308), "ms"),
            "Elapsed_HotWaterTk_CondPump_On":       (get_dint(db3_raw, 312), "ms"),
            "Elapsed_ReturnLine_Pump_On":           (get_dint(db3_raw, 316), "ms"),
            "Elapsed_HotWaterTk_ReturnPump_On":     (get_dint(db3_raw, 320), "ms"),
        }

        # --- OUTPUT ---
        print("\n=== [DB2 CONFIGURATIONS] ===")
        for key, (val, unit) in db2_data.items():
            print(f"{key:35} : {val} {unit}")

        print("\n=== [DB3 LIVE DATA] ===")
        for key, (val, unit) in db3_data.items():
            print(f"{key:35} : {val} {unit}")

    except Exception as e:
        print(f"Error: {e}")

    finally:
        if client.get_connected():
            client.disconnect()


if __name__ == "__main__":
    get_plc_data()
