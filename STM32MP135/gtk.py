import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk
 
class STM32App(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="STM32 GTK Test")
        self.set_default_size(350, 200)
        self.set_border_width(10)
 
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        self.add(vbox)
 
        label = Gtk.Label(label="Enter something:")
        vbox.pack_start(label, False, False, 0)
 
        self.entry = Gtk.Entry()
        vbox.pack_start(self.entry, False, False, 0)
 
        button = Gtk.Button(label="Submit")
        button.connect("clicked", self.on_submit_clicked)
        vbox.pack_start(button, False, False, 0)
 
        self.result_label = Gtk.Label(label="")
        self.result_label.set_use_markup(True)
        vbox.pack_start(self.result_label, False, False, 0)
 
    def on_submit_clicked(self, widget):
        text = self.entry.get_text()
        self.result_label.set_markup(
            f'<span foreground="blue">You typed: {text}</span>'
        )
 
win = STM32App()
win.connect("destroy", Gtk.main_quit)
win.show_all()
Gtk.main()
