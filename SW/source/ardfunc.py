from tkinter import messagebox
import struct
from datetime import datetime


def read_from_master(ser_com):
    buf = []
    buf.extend(ser_com.read())
    if len(buf) == 0:
        messagebox.showwarning("Warring", "no response")
        return None
    elif buf[0] == 0:
        messagebox.showwarning("Warring", "zero length massage")
        return None
    else:
        for index in range(0, buf[0]):
            buf.extend(ser_com.read())

        if buf[-1] != 250:
            messagebox.showwarning("Warring", "Corrupt massage from master {}.".format(len(buf)))

        return buf


def recalib_tank_sensor(ser_com):
    send_d = [1, 0, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Calibrated successfully.")

    else:
        messagebox.showwarning("Failed", "Calibration failed.")


def set_time(ser_com, sec, mins, hour, day, month, year):
    send_d = [2, sec, mins, hour, day, month, year]
    ser_com.write(send_d)

    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Time set successfully.")

    else:
        messagebox.showwarning("Failed", "Time set failed.")


def change_pump_const(ser_com, pconst):
    send_d = [3, pconst, 0, 0, 0, 0, 0]
    ser_com.write(send_d)

    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Pump constant set successfully.")

    else:
        messagebox.showwarning("Failed", "Pump constant set failed.")


def change_pots_param(ser_com, pot1, pot2, pot3, pot4, pot5, pot6):
    send_d = [4, pot1, pot2, pot3, pot4, pot5, pot6]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "pots assign to slaves successfully.")

    else:
        messagebox.showwarning("Failed", "pots assignment failed.")


def set_pot_cd(ser_com, pot_id, cd_time):
    send_d = [5, pot_id, cd_time, 0, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Water amout per shot changed for pot {}.".format(pot_id + 1))

    else:
        messagebox.showwarning("Failed", "Water amout per shot change failed.")


def set_pot_water_amount(ser_com, pot_id, wat_amo):
    [lsb, msb] = int_split(wat_amo)
    send_d = [6, pot_id, lsb, msb, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Cooldown time changed for pot {}.".format(pot_id + 1))

    else:
        messagebox.showwarning("Failed", "Cooldown time set failed.")


def set_pot_moist_level(ser_com, pot_id, moi_lvl_lsb, moi_lvl_msb):
    send_d = [7, pot_id, moi_lvl_lsb, moi_lvl_msb, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Minimum soil moisture level is changed for pot {}.".format(pot_id + 1))

    else:
        messagebox.showwarning("Failed", "Moisture level set failed.")


def clean_master_his(ser_com):
    send_d = [8, 0, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Master log history erased")

    else:
        messagebox.showwarning("Failed", "Failed to erase history.")


def clean_slave_his(ser_com, salve):
    send_d = [9, salve, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Slave log history erased")

    else:
        messagebox.showwarning("Failed", "Failed to erase history.")


def change_water_mode(ser_com, mode):
    send_d = [10, mode, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Mode changed.")

    else:
        messagebox.showwarning("Failed", "Mode change has failed.")


def slave_sensors(ser_com, slave):
    send_d = [11, slave, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    buf = read_from_master(ser_com)
    return buf


def pot_sensors(ser_com, pot):
    send_d = [12, pot, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    buf = read_from_master(ser_com)
    return buf


def dump_history(ser_com, slave=False):
    buf_fullhis = []
    if slave is True:
        send_d = [13, 0, 0]
    else:
        send_d = [13, 0, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    buf = read_from_master(ser_com)

    if buf is not None:
        if buf[0] == 3 and buf[3] == 250:
            his_point = int_recalc(buf[1], buf[2])
        else:
            messagebox.showwarning("Failed", "Dump history has failed. Wrong massage.")
            return 1
    else:

        messagebox.showwarning("Failed", "Dump history has failed. No response.")
        return 1

    if slave is True:
        his_msg_count = (his_point - 4) // 16
    else:
        his_msg_count = (his_point - 44) // 16

    for index in range(0, his_msg_count):
        if slave is True:
            send_d = [13, 1, index]
        else:
            send_d = [13, 1, index, 0, 0, 0, 0]
        ser_com.write(send_d)
        resp_masg = read_from_master(ser_com)

        if resp_masg is None:
            return 1
        else:
            if resp_masg[0] != 17 and resp_masg[-1] == 250:
                return 1

        buf_fullhis.append(resp_masg[1:-1])

    now = datetime.now()
    with open("histoy log " + now.strftime("%d-%m-%Y %H-%M") + ".txt", "w") as file:
        wirte_history_file(file, buf_fullhis)
    messagebox.showinfo("Success", "History log dumped.")
    return 0


def lockout_reset(ser_com):
    send_d = [14, 0, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Lockout reset.")

    else:
        messagebox.showwarning("Failed", "Lockout reset has failed.")


def read_param(ser_com):
    send_d = [15, 0, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    buf = read_from_master(ser_com)
    return buf


def force_water(ser_com, potid):
    send_d = [16, potid, 0, 0, 0, 0, 0]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Watering operation success.")

    else:
        messagebox.showwarning("Failed", "Watering operation failed.")


def config_slave(ser_com, slave_id, no_pots):
    send_d = [1, slave_id, no_pots]
    ser_com.write(send_d)
    if read_from_master(ser_com) == [2, 55, 250]:
        messagebox.showinfo("Success", "Slave configuration successful")

    else:
        messagebox.showwarning("Failed", "Slave configuration failed.")


def int_recalc(lsb, msb):
    return lsb + (256 * msb)


def int_split(int_value):
    msb = int_value // 256
    lsb = int_value - (msb * 256)

    return [lsb, msb]


def byte_to_float(value_list):
    b_value = bytearray(4)
    b_value[:] = value_list

    return struct.unpack('f', b_value)[0]


def wirte_history_file(file, data):

    for index, chank in enumerate(data):

        file.write(str(index + 1))
        file.write(" {}-{}-{} {}:{} ".format(chank[0], chank[1], chank[2], chank[3], chank[4]))

        if chank[5] == 0:
            LOCKOUT = chank[6]
            file.write(" LOCKOUT ISSUED: {}.\n".format(LOCKOUT))

        elif chank[5] == 1:

            pot = chank[6]
            WATER = int_recalc(chank[7], chank[8])
            file.write(" WATERED pot {} WITH {} mL OF WATER.\n".format(pot, WATER))

        elif chank[5] == 2:
            pot = chank[6]
            file.write(" WATERING OF pot {} HAS FAILED.\n".format(pot))

        elif chank[5] == 3:
            SLAVE = chank[6]
            LIGHT = chank[7]
            AIRT = round(byte_to_float(chank[8:12]), 2)
            AIRH = round(byte_to_float(chank[12:16]), 2)
            file.write(" SENSORS OF SLAVE {}: LIGHT: {} LUX,"
                       " AIR TEMPERATURE: {} C, AIR HUMIDITY: {} %.\n".format(SLAVE, LIGHT, AIRT, AIRH))

        elif chank[5] == 4:
            pot = chank[6]
            SOILT = round(byte_to_float(chank[7:11]), 2)
            SOILM = int_recalc(chank[11], chank[12])
            file.write(" SENSORS OF pot {}: SOIL TEMPERATURE: {} C, SOIL MOIST: {}.\n".format(pot, SOILT, SOILM))
