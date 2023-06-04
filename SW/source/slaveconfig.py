import serial
from ardfunc import *
from tkinter import *
from tkinter import messagebox

serial_port = None

root = Tk()
root.title("Slave configuration")


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
    com_entry_port.insert(0, "3")
    com_entry_port.grid(row=0, column=1)
    com_label_speed = Label(root, text="Enter COM port speed:")
    com_label_speed.grid(row=1, column=0)
    com_entry_speed = Entry(root)
    com_entry_speed.insert(0, "9600")
    com_entry_speed.grid(row=1, column=1)
    com_button = Button(root, text="OK", command=lambda: open_com(com_entry_port, com_entry_speed))
    com_button.grid(row=2)


def mainwindow():
    global serial_port
    def send():
        value = int(num_entry.get())
        if value >= 1 <= 255:

            config_slave(serial_port, id_names.index(id_selected.get()), value)
        else:
            messagebox.showwarning("Warning", "Incorrect data, value must be between 1-255.")

    top_text = Label(root, text="Configurate a salve")
    top_text.grid(row=0, column=0, columnspan=2)

    id_text = Label(root, text="Assign ID to slave: ")
    id_text.grid(row=1, column=0)
    id_names = ["ID 1", "ID 2", "ID 3", "ID 4", "ID 5", "ID 6"]
    id_selected = StringVar(root)
    id_selected.set(id_names[0])
    id_select = OptionMenu(root, id_selected, *id_names)
    id_select.grid(row=1, column=1)

    num_text = Label(root, text="Enter number of pods assigned to this slave: ")
    num_text.grid(row=2, column=0)

    num_entry = Entry(root)
    num_entry.insert(0, "1")
    num_entry.grid(row=2, column=1)

    send_button = Button(root, text="Send", command=send)
    send_button.grid(row=3, column=0, columnspan=2)
    label = Label(root)
    label.grid(row=4, column=0)
    his_button = Button(root, text="Dump history log to file", command=lambda: dump_history(serial_port, slave=True))
    his_button.grid(row=5, column=0)
    chis_button = Button(root, text="Clear history on this slave", command=clean_master_his)
    chis_button.grid(row=5, column=1)


startwindow()
root.mainloop()
