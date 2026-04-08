import json
import csv

TAGS = ["full block", "top", "bottom", "directional", "cross", "entity", "animated", "TODO", "unused"]

def json_to_csv_taglist(json_file, csv_file):
    with open(json_file, "r") as f:
        data = json.load(f)

    with open(csv_file, "w", newline="") as f:
        writer = csv.writer(f)

        for filename, tag_list in data.items():
            row = [filename, len(tag_list)] + tag_list
            writer.writerow(row)

if __name__ == "__main__":
    json_to_csv_taglist("pre_parsing/texture_tags.json", "pre_parsing/texture_tags.csv")