from pathlib import Path
import argparse

parser = argparse.ArgumentParser(description="Generate README.md for JVFMT")
parser.add_argument("template_path")
parser.add_argument("dest_path")
args = parser.parse_args()

template_path = Path(args.template_path).resolve()
dest_path = Path(args.dest_path).resolve()

#template_path = Path("docs/readme-template.md").resolve()
#dest_path = Path("README.md").resolve()

def read_file_into_md(p: Path):
    content = p.read_text().splitlines()
    suffix = p.suffix.removeprefix(".")
    if suffix == "md":
        return content
    
    content_2 = []
    in_md = True
    nb_empty_lines = 0
    for line in content:
        line = line.strip()
        if not line:
            nb_empty_lines += 1
            continue
        if line.startswith("///"):
            if not in_md:
                in_md = True
                content_2.append("```")
            content_2 += [""] * nb_empty_lines
            nb_empty_lines = 0
            content_2.append(line[3:].strip())
        else:
            content_2 += [""] * nb_empty_lines
            nb_empty_lines = 0
            if in_md:
                in_md = False
                content_2.append(f"```{suffix}")
            content_2.append(line)
    if not in_md:
        content_2.append("```")
    return content_2

def preprocess_file(path: Path, section: str):
    content = read_file_into_md(path)
    content_2 = []
    in_section = (section == "*")
    for line in content:
        if not (len(line) > 6 and line.startswith("===") and line.endswith("===")):
            if in_section:
                content_2.append(line)
            continue
        args = line[3:-3].split()
        if args[0] == "INCLUDE":
            child_path = args[1]
            child_section = args[2]
            child_content = preprocess_file(path.parent / child_path, child_section)
            content_2 += child_content
        elif args[0] == "BEGIN":
            in_section = (args[1] == section or section == "*")
        elif args[0] == "END":
            in_section = (section == "*")
    return content_2


    

open(dest_path, "w").write("\n".join(preprocess_file(template_path, "*")))


