import serial
from ardfunc import *
from tkinter import *
from tkinter import messagebox

serial_port = None

root = Tk()
root.title("Flower Watering System")


def startwindow():
    def open_com(port, speed):
        global serial_port
        try:
            serial_port = serial.Serial("COM{}".format(port.get(), int(speed.get())))
        except serial.serialutil.SerialException:
            messagebox.showerror("Error", "Can't open this COM port.")
        if serial_port is not None:
            for child in root.winfo_children():
                child.destroy()
                mainwindow()

    com_label_port = Label(root, text="Enter COM port number:")
    com_label_port.grid(row=0, column=0)
    com_entry_port = Entry(root)
    com_entry_port.insert(0, "4")
    com_entry_port.grid(row=0, column=1)
    com_label_speed = Label(root, text="Enter COM port speed:")
    com_label_speed.grid(row=1, column=0)
    com_entry_speed = Entry(root)
    com_entry_speed.insert(0, "9600")
    com_entry_speed.grid(row=1, column=1)
    com_button = Button(root, text="OK", command=lambda: open_com(com_entry_port, com_entry_speed))
    com_button.grid(row=2)


def time_pop():
    def send():

        mins = mins_var.get()
        hours = hours_var.get()
        day = day_var.get()
        month = month_var.get()
        year = year_var.get()

        def check_data():

            if mins < 0 or mins > 59:
                return False
            elif hours < 0 or hours > 23:
                return False
            elif day < 1 or day > 31:
                return False
            elif month < 1 or month > 12:
                return False
            elif year < 0 or year > 99:
                return False
            else:
                return True

        if check_data() is True:
            set_time(serial_port, 0, mins, hours, day, month, year)
            exit_wind()
        else:
            messagebox.showwarning("Warning", "Incorrect data.")

    def exit_wind():
        time_wind.destroy()
        time_wind.update()

    time_wind = Toplevel()
    time_wind.title("Time setup")

    day_text = Label(time_wind, text="Day")
    day_text.grid(row=0, column=0)
    day_var = IntVar(time_wind)
    day_var.set(1)

    choices_days = []
    for x in range(1, 32):
        choices_days.append(x)

    day_menu = OptionMenu(time_wind, day_var, *choices_days)
    day_menu.grid(row=1, column=0)

    month_text = Label(time_wind, text="Month")
    month_text.grid(row=0, column=1)

    month_var = IntVar(time_wind)
    month_var.set(1)

    choices_month = []
    for x in range(1, 13):
        choices_month.append(x)

    month_menu = OptionMenu(time_wind, month_var, *choices_month)
    month_menu.grid(row=1, column=1)

    year_text = Label(time_wind, text="Year")
    year_text.grid(row=0, column=2)

    year_var = IntVar(time_wind)
    year_entry = Entry(time_wind, textvariable=year_var)
    year_var.set(20)
    year_entry.grid(row=1, column=2)

    time_text = Label(time_wind, text="Time", justify=CENTER)
    time_text.grid(row=2, column=0, columnspan=3)

    hours_var = IntVar(time_wind)
    hours_entry = Entry(time_wind,   textvariable=hours_var)
    hours_var.set(0)
    hours_entry.grid(row=3, column=0)

    time_text = Label(time_wind,   text=":")
    time_text.grid(row=3, column=1)

    mins_var = IntVar(time_wind)
    mins_entry = Entry(time_wind, textvariable=mins_var)
    mins_var.set(0)
    mins_entry.grid(row=3, column=2)

    ok_button = Button(time_wind, text="OK", command=send)
    ok_button.grid(row=4, column=0)
    cancel_button = Button(time_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=4, column=2)


def const_pop():
    def send():
        value = const_var.get()
        if 1 <= value <= 255:

            change_pump_const(serial_port, value)
            exit_wind()
        else:
            messagebox.showwarning("Warning", "Incorrect data, value must be between 1-255.")

    def exit_wind():
        const_wind.destroy()
        const_wind.update()

    const_wind = Toplevel()
    const_wind.title("Pump constant change")

    pump_text = Label(const_wind, text="Enter new pump constant:")
    pump_text.grid(row=0, column=0)

    const_var = IntVar(const_wind)
    const_entry = Entry(const_wind, textvariable=const_var)
    const_var.set(13)
    const_entry.grid(row=1, column=0)

    ok_button = Button(const_wind, text="OK", command=send)
    ok_button.grid(row=2, column=0)
    cancel_button = Button(const_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=2, column=1)


def potreconfig_pop():
    def send():
        values = [pot1_var.get(), pot2_var.get(), pot3_var.get(), pot4_var.get(), pot5_var.get(), pot6_var.get()]

        if max(values) <= 255 and min(values) >= 0:

            change_pots_param(serial_port, values[0], values[1], values[2], values[3], values[4], values[5])
            exit_wind()
        else:
            messagebox.showwarning("Warning", "Incorrect data, value must be between 0-255.")

    def exit_wind():
        potreconf_wind.destroy()
        potreconf_wind.update()

    potreconf_wind = Toplevel()
    potreconf_wind.title("pot assignment")

    entry1state = StringVar(potreconf_wind)
    entry1state.set('normal')

    top_text = Label(potreconf_wind, text="Enter slave number for each pot. 255 = pot is disabled")
    top_text.grid(row=0, column=0, columnspan=2)

    pot1_text = Label(potreconf_wind, text="pot1")
    pot1_text.grid(row=1, column=0)
    pot1_var = IntVar(potreconf_wind)
    pot1_var.set(255)
    pot1_entry = Entry(potreconf_wind, textvariable=pot1_var, state=entry1state.get())
    pot1_entry.grid(row=1, column=1)

    pot2_text = Label(potreconf_wind, text="pot2")
    pot2_text.grid(row=2, column=0)
    pot2_var = IntVar(potreconf_wind)
    pot2_var.set(255)
    pot2_entry = Entry(potreconf_wind, textvariable=pot2_var)
    pot2_entry.grid(row=2, column=1)

    pot3_text = Label(potreconf_wind, text="pot3")
    pot3_text.grid(row=3, column=0)
    pot3_var = IntVar(potreconf_wind)
    pot3_var.set(255)
    pot3_entry = Entry(potreconf_wind, textvariable=pot3_var)
    pot3_entry.grid(row=3, column=1)

    pot4_text = Label(potreconf_wind, text="pot4")
    pot4_text.grid(row=4, column=0)
    pot4_var = IntVar(potreconf_wind)
    pot4_var.set(255)
    pot4_entry = Entry(potreconf_wind, textvariable=pot4_var)
    pot4_entry.grid(row=4, column=1)

    pot5_text = Label(potreconf_wind, text="pot5")
    pot5_text.grid(row=5, column=0)
    pot5_var = IntVar(potreconf_wind)
    pot5_var.set(255)

    pot5_entry = Entry(potreconf_wind, textvariable=pot5_var)
    pot5_entry.grid(row=5, column=1)

    pot6_text = Label(potreconf_wind, text="pot6")
    pot6_text.grid(row=6, column=0)
    pot6_var = IntVar(potreconf_wind)
    pot6_var.set(255)
    pot6_entry = Entry(potreconf_wind, textvariable=pot6_var)
    pot6_entry.grid(row=6, column=1)

    ok_button = Button(potreconf_wind, text="OK", command=send)
    ok_button.grid(row=7, column=0)
    cancel_button = Button(potreconf_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=7, column=1)


def cd_pop():
    def send():
        value = int(value_entry.get())
        if 0 <= value <= 255:

            set_pot_cd(serial_port, pots_names.index(pot_selected.get()), value)
        else:
            messagebox.showwarning("Warning", "Incorrect data, value must be between 0-255.")

    def exit_wind():
        cd_wind.destroy()
        cd_wind.update()

    cd_wind = Toplevel()
    cd_wind.title("Cooldown time set")

    pots_names = ["pot1", "pot2", "pot3", "pot4", "pot5", "pot6"]

    pot_selected = StringVar(cd_wind)
    pot_selected.set(pots_names[0])
    top_text = Label(cd_wind, text="Cooldown time set")
    top_text.grid(row=0, column=0, columnspan=2)

    pot_text = Label(cd_wind, text="Select pot: ")
    pot_text .grid(row=1, column=0)

    pot_select = OptionMenu(cd_wind, pot_selected, *pots_names)
    pot_select.grid(row=1, column=1)

    value_text = Label(cd_wind, text="Enter cooldown time in hours: ")
    value_text.grid(row=2, column=0)

    value_entry = Entry(cd_wind)
    value_entry.insert(0, "1")
    value_entry.grid(row=2, column=1)

    ok_button = Button(cd_wind, text="OK", command=send)
    ok_button.grid(row=3, column=0)
    cancel_button = Button(cd_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=3, column=1)


def water_change_pop():
    def send():
        value = int(value_entry.get())
        if 0 <= value <= 65535:

            set_pot_water_amount(serial_port, pots_names.index(pot_selected.get()), value)
        else:
            messagebox.showwarning("Warning", "Incorrect data, value must be between 0-255.")

    def exit_wind():
        wat_wind.destroy()
        wat_wind.update()

    wat_wind = Toplevel()
    wat_wind.title("Amount of water per shot set")

    pots_names = ["pot1", "pot2", "pot3", "pot4", "pot5", "pot6"]

    pot_selected = StringVar(wat_wind)
    pot_selected.set(pots_names[0])
    top_text = Label(wat_wind, text="Amount of water per shot set")
    top_text.grid(row=0, column=0, columnspan=2)

    pot_text = Label(wat_wind, text="Select pot: ")
    pot_text.grid(row=1, column=0)

    pot_select = OptionMenu(wat_wind, pot_selected, *pots_names)
    pot_select.grid(row=1, column=1)

    value_text = Label(wat_wind, text="Enter water amount in ml: ")
    value_text.grid(row=2, column=0)

    value_entry = Entry(wat_wind)
    value_entry.insert(0, "100")
    value_entry.grid(row=2, column=1)

    ok_button = Button(wat_wind, text="OK", command=send)
    ok_button.grid(row=3, column=0)
    cancel_button = Button(wat_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=3, column=1)


def show_param_pop():
    def exit_wind():
        param_wind.destroy()
        param_wind.update()

    param_wind = Toplevel()
    param_wind.title("Parameters")
    params = read_param(serial_port)

    up_text = Label(param_wind, text="Current parameters")
    up_text.grid(row=0, column=0, columnspan=4)
    lim_text = Label(param_wind, text="Supply tank distance limit: ")
    lim_text.grid(row=1, column=0)

    lim_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[1]))
    lim_value.grid(row=1, column=1)

    lock_text = Label(param_wind, text="Lockout: ")
    lock_text.grid(row=2, column=0)

    lock_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[2]))
    lock_value.grid(row=2, column=1)

    pump_text = Label(param_wind, text="Pump constant: ")
    pump_text.grid(row=3, column=0)

    pump_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[3]))
    pump_value.grid(row=3, column=1)

    pot_text = Label(param_wind, text="Number of active pots: ")
    pot_text.grid(row=4, column=0)

    pot_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[4]))
    pot_value.grid(row=4, column=1)

    slave_text = Label(param_wind, text="Number of active slaves: ")
    slave_text.grid(row=5, column=0)

    slave_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[5]))
    slave_value.grid(row=5, column=1)

    cd1_text = Label(param_wind, text="Cooldown time for pot 1: ")
    cd1_text.grid(row=1, column=2)

    cd1_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[6]))
    cd1_value.grid(row=1, column=3)

    cd2_text = Label(param_wind, text="Cooldown time for pot 2: ")
    cd2_text.grid(row=2, column=2)

    cd2_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[7]))
    cd2_value.grid(row=2, column=3)

    cd3_text = Label(param_wind, text="Cooldown time for pot 3: ")
    cd3_text.grid(row=3, column=2)

    cd3_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[8]))
    cd3_value.grid(row=3, column=3)

    cd4_text = Label(param_wind, text="Cooldown time for pot 4: ")
    cd4_text.grid(row=4, column=2)

    cd4_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[9]))
    cd4_value.grid(row=4, column=3)

    cd5_text = Label(param_wind, text="Cooldown time for pot 5: ")
    cd5_text.grid(row=5, column=2)

    cd5_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[10]))
    cd5_value.grid(row=5, column=3)

    cd6_text = Label(param_wind, text="Cooldown time for pot 6: ")
    cd6_text.grid(row=6, column=2)

    cd6_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[11]))
    cd6_value.grid(row=6, column=3)

    water1_text = Label(param_wind, text="Water amount per shot for pot 1: ")
    water1_text.grid(row=7, column=0)

    water1_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[12], params[13])))
    water1_value.grid(row=7, column=1)

    water2_text = Label(param_wind, text="Water amount per shot for pot 2: ")
    water2_text.grid(row=8, column=0)

    water2_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[14], params[15])))
    water2_value.grid(row=8, column=1)

    water3_text = Label(param_wind, text="Water amount per for pot 3: ")
    water3_text.grid(row=9, column=0)

    water3_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[16], params[17])))
    water3_value.grid(row=9, column=1)

    water4_text = Label(param_wind, text="Water amount per for pot 4: ")
    water4_text.grid(row=10, column=0)

    water4_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[18], params[19])))
    water4_value.grid(row=10, column=1)

    water5_text = Label(param_wind, text="Water amount per for pot 5: ")
    water5_text.grid(row=11, column=0)

    water5_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[20], params[21])))
    water5_value.grid(row=11, column=1)

    water6_text = Label(param_wind, text="Water amount per for pot 6: ")
    water6_text.grid(row=12, column=0)

    water6_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[22], params[23])))
    water6_value.grid(row=12, column=1)

    moist1_text = Label(param_wind, text="Minimum soil moisture level for pot 1: ")
    moist1_text.grid(row=7, column=2)

    moist1_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[24], params[25])))
    moist1_value.grid(row=7, column=3)

    moist2_text = Label(param_wind, text="Minimum soil moisture level for pot 2: ")
    moist2_text.grid(row=8, column=2)

    moist2_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[26], params[27])))
    moist2_value.grid(row=8, column=3)

    moist3_text = Label(param_wind, text="Minimum soil moisture level for pot 3: ")
    moist3_text.grid(row=9, column=2)

    moist3_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[28], params[29])))
    moist3_value.grid(row=9, column=3)

    moist4_text = Label(param_wind, text="Minimum soil moisture level for pot 4: ")
    moist4_text.grid(row=10, column=2)

    moist4_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[30], params[31])))
    moist4_value.grid(row=10, column=3)

    moist5_text = Label(param_wind, text="Minimum soil moisture level for pot 5: ")
    moist5_text.grid(row=11, column=2)

    moist5_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[32], params[33])))
    moist5_value.grid(row=11, column=3)

    moist6_text = Label(param_wind, text="Minimum soil moisture level for pot 6: ")
    moist6_text.grid(row=12, column=2)

    moist6_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[34], params[35])))
    moist6_value.grid(row=12, column=3)

    potid1_text = Label(param_wind, text="Slave number for pot 1 (255 = pot is disabled): ")
    potid1_text.grid(row=13, column=0)

    potid1_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[36]))
    potid1_value.grid(row=13, column=1)

    potid2_text = Label(param_wind, text="Slave number for pot 2 (255 = pot is disabled): ")
    potid2_text.grid(row=14, column=0)

    potid2_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[37]))
    potid2_value.grid(row=14, column=1)

    potid3_text = Label(param_wind, text="Slave number for pot 3 (255 = pot is disabled): ")
    potid3_text.grid(row=15, column=0)

    potid3_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[38]))
    potid3_value.grid(row=15, column=1)

    potid4_text = Label(param_wind, text="Slave number for pot 4 (255 = pot is disabled): ")
    potid4_text.grid(row=16, column=0)

    potid4_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[39]))
    potid4_value.grid(row=16, column=1)

    potid5_text = Label(param_wind, text="Slave number for pot 5 (255 = pot is disabled): ")
    potid5_text.grid(row=17, column=0)

    potid5_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[40]))
    potid5_value.grid(row=17, column=1)

    potid6_text = Label(param_wind, text="Slave number for pot 6 (255 = pot is disabled): ")
    potid6_text.grid(row=18, column=0)

    potid6_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[41]))
    potid6_value.grid(row=18, column=1)

    watermode_text = Label(param_wind, text="Water mode: ")
    watermode_text.grid(row=6, column=0)

    watermode_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(params[42]))
    watermode_value.grid(row=6, column=1)

    hism_text = Label(param_wind, text="History memory pointer: ")
    hism_text.grid(row=19, column=0)

    hism_value = Label(param_wind, bd=1, relief=SUNKEN, text=str(int_recalc(params[43], params[44])))
    hism_value.grid(row=19, column=1)

    cancel_button = Button(param_wind, text="OK", command=exit_wind)
    cancel_button.grid(row=20, column=0, columnspan=4)


def change_mode_pop():
    def send():
        change_water_mode(serial_port, mods_names.index(mode_selected.get()))
        exit_wind()

    def exit_wind():
        mode_wind.destroy()
        mode_wind.update()

    mode_wind = Toplevel()
    mode_wind.title("Cange mode ")

    mods_names = ["No automation", "Cooldown time dependant", "Daytime dependant"]
    mode_selected = StringVar(mode_wind)
    mode_selected.set(mods_names[0])
    top_text = Label(mode_wind, text="Select mode")
    top_text.grid(row=0, column=0, columnspan=2)

    mode_text = Label(mode_wind, text="Select pot: ")
    mode_text.grid(row=1, column=0)

    mode_select = OptionMenu(mode_wind, mode_selected, *mods_names)
    mode_select.grid(row=1, column=1)

    ok_button = Button(mode_wind, text="OK", command=send)
    ok_button.grid(row=2, column=0)
    cancel_button = Button(mode_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=2, column=1)


def moist_pop():
    def send():
        value = int(value_entry.get())
        if 0 <= value <= 65535:
            value_lsb, value_msb = int_split(value)
            set_pot_moist_level(serial_port, pots_names.index(pot_selected.get()), value_lsb, value_msb)
        else:
            messagebox.showwarning("Warning", "Incorrect data, value must be between 0-65535.")

    def exit_wind():
        moist_wind.destroy()
        moist_wind.update()

    moist_wind = Toplevel()
    moist_wind.title("Minimum soil moisture level set")

    pots_names = ["pot1", "pot2", "pot3", "pot4", "pot5", "pot6"]

    pot_selected = StringVar(moist_wind)
    pot_selected.set(pots_names[0])
    top_text = Label(moist_wind, text="Minimum soil moisture level set")
    top_text.grid(row=0, column=0, columnspan=2)

    pot_text = Label(moist_wind, text="Select pot: ")
    pot_text.grid(row=1, column=0)

    pot_select = OptionMenu(moist_wind, pot_selected, *pots_names)
    pot_select.grid(row=1, column=1)

    value_text = Label(moist_wind, text="Enter minimum soil moisture level: ")
    value_text.grid(row=2, column=0)

    value_entry = Entry(moist_wind)
    value_entry.insert(0, "250")
    value_entry.grid(row=2, column=1)

    ok_button = Button(moist_wind, text="OK", command=send)
    ok_button.grid(row=3, column=0)
    cancel_button = Button(moist_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=3, column=1)


def slave_er_pop():
    def send():
        value = id_names.index(id_selected.get())

        clean_slave_his(serial_port, value)

    def exit_wind():
        slave_er_wind.destroy()
        slave_er_wind.update()

    slave_er_wind = Toplevel()
    slave_er_wind.title("Slave history erase")

    top_text = Label(slave_er_wind, text="Slave history erase")
    top_text.grid(row=0, column=0, columnspan=2)

    slave_ertext = Label(slave_er_wind, text="Select slave: ")
    slave_ertext.grid(row=1, column=0)

    id_names = ["ID 1", "ID 2", "ID 3", "ID 4", "ID 5", "ID 6"]
    id_selected = StringVar(slave_er_wind)
    id_selected.set(id_names[0])
    id_select = OptionMenu(slave_er_wind, id_selected, *id_names)
    id_select.grid(row=1, column=1)

    ok_button = Button(slave_er_wind, text="OK", command=send)
    ok_button.grid(row=2, column=0)
    cancel_button = Button(slave_er_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=2, column=1)


def fwater_pop():
    def send():
        pot = pot_names.index(pot_selected.get())

        force_water(serial_port, pot)

    def exit_wind():
        slave_er_wind.destroy()
        slave_er_wind.update()

    slave_er_wind = Toplevel()
    slave_er_wind.title("Force watering")

    top_text = Label(slave_er_wind, text="Force watering")
    top_text.grid(row=0, column=0, columnspan=2)

    slave_ertext = Label(slave_er_wind, text="Select pot to water: ")
    slave_ertext.grid(row=1, column=0)

    pot_names = ["pot 1", "pot 2", "pot 3", "pot 4", "pot 5", "pot 6"]
    pot_selected = StringVar(slave_er_wind)
    pot_selected.set(pot_names[0])
    pot_select = OptionMenu(slave_er_wind, pot_selected, *pot_names)
    pot_select.grid(row=1, column=1)

    ok_button = Button(slave_er_wind, text="OK", command=send)
    ok_button.grid(row=2, column=0)
    cancel_button = Button(slave_er_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=2, column=1)


def sensors_pop():
    def slave_send():
        data = slave_sensors(serial_port, slave_names.index(slave_selected.get()))
        light_var.set(str(data[1]))

        airt_fvalue = round(byte_to_float(data[2:6]), 2)
        airt_var.set(str(airt_fvalue))

        airh_fvalue = round(byte_to_float(data[6:10]), 2)
        airh_var.set(str(airh_fvalue))

    def pot_send():
        data = pot_sensors(serial_port, pot_names.index(pot_selected.get()))

        soilt_fvalue = round(byte_to_float(data[1:5]), 2)
        soilt_var.set(str(soilt_fvalue))

        soilm_ivalue = int_recalc(data[5], data[6])
        soilm_var.set(str(soilm_ivalue))

    def exit_wind():
        sens_wind.destroy()
        sens_wind.update()

    sens_wind = Toplevel()
    sens_wind.title("Sensors")

    slave_text = Label(sens_wind, text="Select slave: ")
    slave_text.grid(row=0, column=0)
    slave_names = ["Slave 1", "Slave 2", "Slave 3", "Slave 4", "Slave 5", "Slave 6"]
    slave_selected = StringVar(sens_wind)
    slave_selected.set(slave_names[0])

    slave_select = OptionMenu(sens_wind, slave_selected, *slave_names)
    slave_select.grid(row=0, column=1)

    pot_text = Label(sens_wind, text="Select pot: ")
    pot_text.grid(row=0, column=2)
    pot_names = ["pot 1", "pot 2", "pot 3", "pot 4", "pot 5", "pot 6"]
    pot_selected = StringVar(sens_wind)
    pot_selected.set(pot_names[0])

    slave_select = OptionMenu(sens_wind, pot_selected, *pot_names)
    slave_select.grid(row=0, column=3)

    light_text = Label(sens_wind, text="Light intensity (lux): ")
    light_text.grid(row=1, column=0)
    light_var = StringVar()
    light_var.set("0")
    light_value = Label(sens_wind, bd=1, relief=SUNKEN, textvariable=light_var)
    light_value.grid(row=1, column=1)

    airt_text = Label(sens_wind, text="Air temperature (C): ")
    airt_text.grid(row=2, column=0)
    airt_var = StringVar()
    airt_var.set("0")
    airt_value = Label(sens_wind, bd=1, relief=SUNKEN, textvariable=airt_var)
    airt_value.grid(row=2, column=1)

    airh_text = Label(sens_wind, text="Air humidity (%): ")
    airh_text.grid(row=3, column=0)
    airh_var = StringVar()
    airh_var.set("0")
    airh_value = Label(sens_wind, bd=1, relief=SUNKEN, textvariable=airh_var)
    airh_value.grid(row=3, column=1)

    soilm_text = Label(sens_wind, text="Soil moisture: ")
    soilm_text.grid(row=1, column=2)
    soilm_var = StringVar()
    soilm_var.set("0")
    soilm_value = Label(sens_wind, bd=1, relief=SUNKEN, textvariable=soilm_var)
    soilm_value.grid(row=1, column=3)

    soilt_text = Label(sens_wind, text="Soil temperature (C): ")
    soilt_text.grid(row=2, column=2)
    soilt_var = StringVar()
    soilt_var.set("0")
    soilt_value = Label(sens_wind, bd=1, relief=SUNKEN, textvariable=soilt_var)
    soilt_value.grid(row=2, column=3)

    slave_button = Button(sens_wind, text="Read slave sensors", command=slave_send)
    slave_button.grid(row=4, column=0)
    pot_button = Button(sens_wind, text="Read pot sensors", command=pot_send)
    pot_button.grid(row=4, column=2)
    cancel_button = Button(sens_wind, text="Cancel", command=exit_wind)
    cancel_button.grid(row=4, column=3)


def mainwindow():
    global serial_port
    buttonh = 5
    buttonw = 25
    calib_button = Button(root, text="Calibrate Supply Tank", height=buttonh, width=buttonw, command=lambda: recalib_tank_sensor(serial_port))
    calib_button.grid(row=0, column=0)
    time_button = Button(root, text="Set time",height=buttonh, width=buttonw, command=time_pop)
    time_button.grid(row=1, column=0)
    pconst_button = Button(root, text="Set pump const",height=buttonh, width=buttonw, command=const_pop)
    pconst_button.grid(row=2, column=0)
    pot_button = Button(root, text="Assign pots to slaves",height=buttonh, width=buttonw, command=potreconfig_pop)
    pot_button.grid(row=0, column=1)
    cd_button = Button(root, text="Set time for cooldowns",height=buttonh, width=buttonw, command=cd_pop)
    cd_button.grid(row=1, column=1)
    water_button = Button(root, text="Change amount of water",height=buttonh, width=buttonw, command=water_change_pop)
    water_button.grid(row=2, column=1)
    moist_button = Button(root, text="Set minimum moisture level",height=buttonh, width=buttonw, command=moist_pop)
    moist_button.grid(row=0, column=2)
    mhis_button = Button(root, text="Erase master histroy",height=buttonh, width=buttonw, command=lambda: clean_master_his(serial_port))
    mhis_button.grid(row=1, column=2)
    shis_button = Button(root, text="Erase slave histroy",height=buttonh, width=buttonw, command=slave_er_pop)
    shis_button.grid(row=2, column=2)
    mdum_button = Button(root, text="Dump histroy",height=buttonh, width=buttonw, command=lambda: dump_history(serial_port))
    mdum_button.grid(row=0, column=3)
    shows_button = Button(root, text="Show sensors values",height=buttonh, width=buttonw, command=sensors_pop)
    shows_button.grid(row=1, column=3)
    showp_button = Button(root, text="Show parameters",height=buttonh, width=buttonw, command=show_param_pop)
    showp_button.grid(row=2, column=3)
    mode_button = Button(root, text="Change mode",height=buttonh, width=buttonw, command=change_mode_pop)
    mode_button.grid(row=0, column=4)
    fwater_button = Button(root, text="Force water",height=buttonh, width=buttonw, command=fwater_pop)
    fwater_button.grid(row=1, column=4)
    lock_button = Button(root, text="Reset Lockout",height=buttonh, width=buttonw, command=lambda: lockout_reset(serial_port))
    lock_button.grid(row=2, column=4)


startwindow()
root.mainloop()
