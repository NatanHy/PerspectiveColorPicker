from PIL import ImageGrab
import hashlib
import os
import time

CLIPBOARD_DIR = "clipboard"
IMAGE_PATH = os.path.join(CLIPBOARD_DIR, "clipboard.png")
META_PATH = os.path.join(CLIPBOARD_DIR, "clipboard.meta")
STOP_FILE = "clipboard/stop"


POLL_INTERVAL = 0.5  # seconds

os.makedirs(CLIPBOARD_DIR, exist_ok=True)

def get_image_hash(img):
    return hashlib.md5(img.tobytes()).hexdigest()

def load_previous_hash():
    if not os.path.exists(META_PATH):
        return None
    with open(META_PATH, "r") as f:
        return f.read().strip()

def save_hash(h):
    with open(META_PATH, "w") as f:
        f.write(h)

last_hash = load_previous_hash()

print("Clipboard watcher started...")

while True:
    try:
        img = ImageGrab.grabclipboard()

        if img is not None:
            current_hash = get_image_hash(img)

            if current_hash != last_hash:
                img.save(IMAGE_PATH)
                time.sleep(0.5)
                save_hash(current_hash)

                last_hash = current_hash
                print("Clipboard image updated.")

        time.sleep(POLL_INTERVAL)

    except Exception as e:
        print("Error:", e)
        time.sleep(1)