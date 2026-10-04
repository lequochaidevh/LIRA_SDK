#!/usr/bin/env python3
import os
import sys
import xml.etree.ElementTree as ET
from jinja2 import Environment, FileSystemLoader

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
    
    for msg in root.findall('message'):
        msg_name = msg.get('name').upper()
        msgid = msg.get('id')
        
        fields = []
        msg_size = 0
        for f in msg.findall('field'):
            f_type = f.get('type')
            f_name = f.get('name')
            f_desc = f.text.strip() if f.text else ""
            fields.append({'type': f_type, 'name': f_name, 'desc': f_desc})
            msg_size += TYPE_SIZES.get(f_type, 0)
            
        parsed_messages.append({
            'name': msg_name,
            'id': msgid,
            'fields': fields,
            'size': msg_size
        })
    return parsed_messages

def run_pipeline(xml_input, output_dir):
    # Setup Jinja environment
    template_dir = os.path.join(os.path.dirname(__file__), 'templates')
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True, lstrip_blocks=True)
    
    messages = parse_xml_schema(xml_input)
    os.makedirs(output_dir, exist_ok=True)
    
    # 1. Output the master tracking config header
    master_template = env.get_get_template('message_def.h.txt') if hasattr(env, 'get_get_template') else env.get_template('message_def.h.txt')
    with open(os.path.join(output_dir, "msg_definition.h"), "w", encoding="utf-8") as f:
        f.write(master_template.render(messages=messages))
    print(f"🟢 Generated master lookup index: msg_definition.h")

    set_temp = env.get_template('message_set.h.txt')
    with open(os.path.join(output_dir, "msg_set.h"), "w") as f:
        f.write(set_temp.render(messages=messages))
    print("🟢 Auto-generated: msg_set.h")
        
    # 2. Output individual static fast-packing C schemas
    msg_template = env.get_template('message_msg.h.txt')
    for msg in messages:
        file_name = f"{msg['name'].lower()}.h"
        with open(os.path.join(output_dir, file_name), "w", encoding="utf-8") as f:
            f.write(msg_template.render(msg=msg))
        print(f"  ├── 📄 Exported: {file_name}")
        

if __name__ == "__main__":
    xml_file = os.path.join(os.path.dirname(__file__), "common.xml")
    target_output_dir = os.path.abspath("../msgid")
    
    run_pipeline(xml_file, target_output_dir)
    print("✨ Dynamic C-Header generation process completed successfully.")