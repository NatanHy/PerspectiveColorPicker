import os
import json

def check_json_files(directory):
    qualifiers = set(['all', 'amethyst', 'back', 'bamboo', 'bar', 'bars', 'base', 'beacon', 'body', 'bottom', 'cactus', 'cactus_top', 'calibrated_side', 'candle', 'cocoa', 'content', 'crop', 'cross', 'cross_emissive', 'dirt', 'down', 'east', 'edge', 'end', 'end_rod', 'eye', 'fan', 'fire', 'flower', 'flowerbed', 'flowerpot', 'front', 'glass', 'glow_lichen', 'hook', 'inner_top', 'inside', 'lantern', 'leaf', 'leg', 'lever', 'line', 'lit', 'lit_log', 'lock', 'log', 'north', 'obsidian', 'overlay', 'pane', 'particle', 'pattern', 'pitcher_bottom', 'pitcher_side', 'pitcher_top', 'pivot', 'plant', 'platform', 'portal', 'post', 'propagule', 'rail', 'round', 'sapling', 'saw', 'sculk_vein', 'side', 'sides', 'slab', 'south', 'stage_1', 'stage_2', 'stage_3_bottom', 'stage_3_top', 'stage_4_bottom', 'stage_4_top', 'stand', 'stem', 'tendrils', 'tentacles', 'texture', 'tip', 'top', 'torch', 'tripwire', 'unlit', 'unsticky', 'up', 'upperstem', 'vine', 'wall', 'west', 'wood', 'wool'])
    textures = {}
    for filename in os.listdir("minecraft/textures/block"):
        if filename.endswith(".png"):
            textures[filename] = set()


    for filename in os.listdir(directory):
        if filename.endswith(".json"):
            filepath = os.path.join(directory, filename)

            try:
                with open(filepath, "r", encoding="utf-8") as f:
                    data = json.load(f)

                if "textures" in data:
                    for qual in data["textures"]:
                        f = data["textures"][qual].split("/")[-1] + ".png"
                        if f in textures:
                            textures[f].add(qual)

            except Exception as e:
                print(f"Error reading {filename}: {e}")

    return textures


# Example usage
directory_path = "minecraft/models/block"

#["texture", "all", "side", "sides", "inner", "pattern", "front", "end", "top", "bottom", "cross", "crop", "fan", "fire", "stem", "hook"]

textures = check_json_files(directory_path)

with open("texture_qualifiers.txt", "w") as f:
    for t, q in textures.items():
        f.write(f"{t} {len(q)} {','.join(q)}\n")