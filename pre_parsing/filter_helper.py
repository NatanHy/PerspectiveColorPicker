import os
import json
import tkinter as tk
from PIL import Image, ImageTk

TEXTURE_DIR = "minecraft/textures/block"
TAGS = ["full block", "top", "bottom", "directional", "cross", "entity", "animated", "TODO", "unused"]
OUTPUT_FILE = "pre_parsing/texture_tags.json"


class TextureTagger:
    def __init__(self, root):
        self.root = root
        self.root.title("Texture Tagger")

        self.files = [f for f in os.listdir(TEXTURE_DIR) if f.endswith((".png"))]
        self.index = 0

        # Load existing data if it exists
        if os.path.exists(OUTPUT_FILE):
            with open(OUTPUT_FILE, "r") as f:
                self.data = json.load(f)
        else:
            self.data = {}

        self.skip_button = tk.Button(root, text="Skip Existing", command=self.skip_textures)
        self.skip_button.pack(anchor="e")

        self.image_label = tk.Label(root)
        self.image_label.pack()

        self.checkbox_vars = {}
        self.checkboxes_frame = tk.Frame(root)
        self.checkboxes_frame.pack()

        for tag in TAGS:
            var = tk.BooleanVar()
            cb = tk.Checkbutton(self.checkboxes_frame, text=tag, variable=var)
            cb.pack(anchor="w")
            self.checkbox_vars[tag] = var

        self.next_button = tk.Button(root, text="Next", command=self.next_texture)
        self.prev_button = tk.Button(root, text="Back", command=self.prev_texture)
        self.next_button.pack(anchor="e")
        self.prev_button.pack(anchor="w")

        self.root.bind("<space>", self.on_space)

        self.load_texture()

    def on_space(self, event):
        self.next_texture()

    def load_texture(self):
        if self.index >= len(self.files):
            self.finish()
            return

        filename = self.files[self.index]
        path = os.path.join(TEXTURE_DIR, filename)

        img = Image.open(path)
        img = img.resize((256, 256))
        self.tk_img = ImageTk.PhotoImage(img)

        self.image_label.config(image=self.tk_img)
        self.root.title(f"Tagging: {filename}")

        # Reset checkboxes
        for var in self.checkbox_vars.values():
            var.set(False)

        # Restore previous selections if they exist
        if filename in self.data:
            if not "unused" in self.data[filename]:
                self.next_texture()
            for tag in self.data[filename]:
                if tag in self.checkbox_vars:
                    self.checkbox_vars[tag].set(True)

    def skip_textures(self):
        filename = self.files[self.index]

        while filename in self.data:
            self.next_texture()
            filename = self.files[self.index]

    def save_data(self):
        with open(OUTPUT_FILE, "w") as f:
            json.dump(self.data, f, indent=4)

    def next_texture(self):
        filename = self.files[self.index]

        selected = [tag for tag, var in self.checkbox_vars.items() if var.get()]
        self.data[filename] = selected

        self.save_data()

        self.index += 1
        self.load_texture()

    def prev_texture(self):
        self.index -= 1
        filename = self.files[self.index]

        selected = [tag for tag, var in self.checkbox_vars.items() if var.get()]
        self.data[filename] = selected

        self.load_texture()

    def finish(self):
        self.save_data()
        self.image_label.config(text="Done!")
        print("Tagging complete. Saved to", OUTPUT_FILE)


if __name__ == "__main__":
    root = tk.Tk()
    app = TextureTagger(root)
    root.mainloop()