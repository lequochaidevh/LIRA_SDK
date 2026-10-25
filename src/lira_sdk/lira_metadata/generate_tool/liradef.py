#!/usr/bin/env python3
import os
import sys
import xml.etree.ElementTree as ET
from jinja2 import Environment, FileSystemLoader
import subprocess

# Type mapping dictionary to calculate lengths
TYPE_SIZES = {
    'uint8_t': 1, 'int8_t': 1,
    'uint16_t': 2, 'int16_t': 2,
    'uint32_t': 4, 'int32_t': 4,
    'uint64_t': 8, 'int64_t': 8,
    'float': 4, 'double': 8
}

def parse_xml_schema(xml_path):
    if not os.path.exists(xml_path):
        print(f"❌ Target input XML not found: {xml_path}")
        sys.exit(1)
        
    tree = ET.parse(xml_path)
    root = tree.getroot()
    
    parsed_messages = []
    parsed_enums = []
    
    # 1. Parse Enums (.// deep search)
    for enum_node in root.findall('.//enum'):
        enum_name = enum_node.get('name').upper()
        entries = []
        for entry in enum_node.findall('entry'):
            e_name = entry.get('name').upper()
            e_val = entry.get('value')
            
            # 1. Check if 'description' attribute exists
            e_desc = entry.get('description')
            
            # 2. If no attribute, fallback to inner text node between <entry>...</entry>
            if not e_desc and entry.text:
                e_desc = entry.text.strip()
                
            # 3. If it's still empty or None, fallback to a clean None instead of an empty string ""
            if not e_desc:
                e_desc = None
                
            entries.append({'name': e_name, 'value': e_val, 'desc': e_desc})
        parsed_enums.append({'name': enum_name, 'entries': entries})
    
    # 2. Parse Messages
    for msg in root.findall('.//message'):
        msg_name = msg.get('name').upper()
        msgid = msg.get('id')
        
        fields = []
        msg_size = 0
        for f in msg.findall('field'):
            f_type = f.get('type')
            f_name = f.get('name')
            f_desc = f.text.strip() if f.text else ""
            
            # Calc array size (ví dụ: char[128] or uint8_t[20])
            base_type = f_type.split('[')[0] if '[' in f_type else f_type
            array_multiplier = 1
            if '[' in f_type:
                try:
                    array_multiplier = int(f_type.split('[')[1].replace(']', ''))
                except ValueError:
                    pass
            
            fields.append({'type': f_type, 'name': f_name, 'desc': f_desc})
            msg_size += (TYPE_SIZES.get(base_type, 0) * array_multiplier)
            
        parsed_messages.append({
            'name': msg_name,
            'id': msgid,
            'fields': fields,
            'size': msg_size
        })
        
    return parsed_messages, parsed_enums

# FUNC1: XML + Template -> File Header only once
def run_pipeline_generic(messages, enums, template_name, output_filename, output_dir):
    template_dir = os.path.join(os.path.dirname(__file__), 'templates')
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True, lstrip_blocks=True)
    
    template = env.get_template(template_name)
    output_path = os.path.join(output_dir, output_filename)

    dir_name = os.path.basename(output_dir)
    # This prevents the TypeError if a path string or empty variable is passed.
    if isinstance(messages, list) and len(messages) > 0 and isinstance(messages[0], dict):
        generated_includes = [f"{dir_name}/{msg['name'].lower()}.h" for msg in messages]
    else:
        generated_includes = []

    # Render template cleanly
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(template.render(messages=messages, enums=enums, includes=generated_includes))
        
    print(f"🟢 [Generic] Generated: {output_filename}")

# FUNC2: Multi header - specifically
def generate_individual_messages(messages, template_name, output_dir):
    template_dir = os.path.join(os.path.dirname(__file__), 'templates')
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True, lstrip_blocks=True)
    
    msg_template = env.get_template(template_name)
    for msg in messages:
        file_name = f"{msg['name'].lower()}.h"
        with open(os.path.join(output_dir, file_name), "w", encoding="utf-8") as f:
            f.write(msg_template.render(msg=msg))
        print(f"  ├── 📄 Exported static schema: {file_name}")

def apply_clang_format(file_path):
    """
    Applies the project's .clang-format configuration onto a specific generated file.
    Safely handles cases where clang-format tool is missing in the system path.
    """
    if not os.path.exists(file_path):
        return False
        
    try:
        # Executes formatting in-place using the closest parent .clang-format file
        subprocess.run(["clang-format", "-i", file_path], check=True)
        print(f"  🎨 Styled clean via clang-format: {os.path.basename(file_path)}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(f"  ⚠️  Skipped formatting: 'clang-format' tool not found in system PATH for {os.path.basename(file_path)}.")
        return False


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    target_output_dir = os.path.abspath(os.path.join(script_dir, "../message_type"))
    os.makedirs(target_output_dir, exist_ok=True)
    
    xml_files = [
        os.path.join(script_dir, "common.xml"),
        os.path.join(script_dir, "command.xml"),
        os.path.join(script_dir, "parameters.xml")
    ]
    
    all_messages = []
    all_enums = []
    
    for xml_path in xml_files:
        if os.path.exists(xml_path):
            msgs, enums = parse_xml_schema(xml_path)
            all_messages.extend(msgs)
            all_enums.extend(enums)
            print(f" Loaded XML definition from: {os.path.basename(xml_path)}")

    print(f" Total cataloged: {len(all_messages)} messages, {len(all_enums)} enums.")
    print("---------------------------------------------------------------------")

    run_pipeline_generic(all_messages, all_enums, "message_def.h.txt", "msg_definition.h", target_output_dir)
    run_pipeline_generic(all_messages, all_enums, "message_set.h.txt", "msg_set.h", target_output_dir)

    print("---------------------------------------------------------------------")
    print(" Deploying static fast-packing C schemas:")
    generate_individual_messages(all_messages, "message_msg.h.txt", target_output_dir)
    
    print("\n Dynamic C-Header generation process completed successfully.")
    xml_file = os.path.join(script_dir, "command.xml")
    run_pipeline_generic(xml_file, all_enums,"command_type.h.txt", "command_type.h", target_output_dir)

    print("\n Dynamic C-Header generation process completed successfully.")
    xml_file = os.path.join(script_dir, "parameters.xml")
    run_pipeline_generic(xml_file, all_enums,"param_config.h.txt", "param_config.h", target_output_dir)

    # Step 2: Cleanly loop through the output directory and format every generated .h file automatically
    for root_dir, _, files in os.walk(target_output_dir):
        for file in files:
            if file.endswith(".h"):
                full_file_path = os.path.join(root_dir, file)
                apply_clang_format(full_file_path)

    print("✅ All tasks finalized successfully.")

