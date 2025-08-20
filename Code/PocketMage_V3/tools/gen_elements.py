#!/usr/bin/env python3
"""
Element data packer for PocketMage Periodic Table app.
Converts JSON element data to packed C arrays for embedded use.
"""

import json
import math
import sys
from collections import OrderedDict

def add_string_to_table(blob, string):
    """Add string to blob and return offset"""
    if not string:
        string = ""
    offset = len(blob)
    blob.extend(string.encode('utf-8'))
    blob.append(0)  # null terminator
    return offset

def scale_value(val, scale, missing=-1):
    """Scale float value to integer, return missing value if None"""
    if val is None or val == "":
        return missing
    try:
        return int(round(float(val) * scale))
    except (ValueError, TypeError):
        return missing

def map_category(cat_str):
    """Map category string to enum value"""
    category_map = {
        'alkali metal': 0,
        'alkaline earth metal': 1,
        'transition metal': 2,
        'post-transition metal': 3,
        'metalloid': 4,
        'reactive nonmetal': 5,
        'noble gas': 6,
        'lanthanoid': 7,
        'actinoid': 8
    }
    return category_map.get(cat_str.lower(), 9)  # 9 = Unknown

def map_block(block_str):
    """Map block string to enum value"""
    block_map = {'s': 0, 'p': 1, 'd': 2, 'f': 3}
    return block_map.get(block_str.lower(), 0)

def generate_sample_data():
    """Generate sample data for first 20 elements"""
    elements = []
    
    # Sample data for first 20 elements
    sample_elements = [
        {"z": 1, "symbol": "H", "name": "Hydrogen", "mass": 1.008, "group": 1, "period": 1, "block": "s", "category": "reactive nonmetal", "mp_k": 14.01, "bp_k": 20.28, "density": 0.00008988, "en": 2.20},
        {"z": 2, "symbol": "He", "name": "Helium", "mass": 4.003, "group": 18, "period": 1, "block": "s", "category": "noble gas", "mp_k": 0.95, "bp_k": 4.22, "density": 0.0001785, "en": None},
        {"z": 3, "symbol": "Li", "name": "Lithium", "mass": 6.941, "group": 1, "period": 2, "block": "s", "category": "alkali metal", "mp_k": 453.69, "bp_k": 1615, "density": 0.534, "en": 0.98},
        {"z": 4, "symbol": "Be", "name": "Beryllium", "mass": 9.012, "group": 2, "period": 2, "block": "s", "category": "alkaline earth metal", "mp_k": 1560, "bp_k": 2742, "density": 1.85, "en": 1.57},
        {"z": 5, "symbol": "B", "name": "Boron", "mass": 10.811, "group": 13, "period": 2, "block": "p", "category": "metalloid", "mp_k": 2349, "bp_k": 4200, "density": 2.34, "en": 2.04},
        {"z": 6, "symbol": "C", "name": "Carbon", "mass": 12.011, "group": 14, "period": 2, "block": "p", "category": "reactive nonmetal", "mp_k": 3915, "bp_k": 4098, "density": 2.267, "en": 2.55},
        {"z": 7, "symbol": "N", "name": "Nitrogen", "mass": 14.007, "group": 15, "period": 2, "block": "p", "category": "reactive nonmetal", "mp_k": 63.15, "bp_k": 77.36, "density": 0.0012506, "en": 3.04},
        {"z": 8, "symbol": "O", "name": "Oxygen", "mass": 15.999, "group": 16, "period": 2, "block": "p", "category": "reactive nonmetal", "mp_k": 54.36, "bp_k": 90.20, "density": 0.001429, "en": 3.44},
        {"z": 9, "symbol": "F", "name": "Fluorine", "mass": 18.998, "group": 17, "period": 2, "block": "p", "category": "reactive nonmetal", "mp_k": 53.53, "bp_k": 85.03, "density": 0.001696, "en": 3.98},
        {"z": 10, "symbol": "Ne", "name": "Neon", "mass": 20.180, "group": 18, "period": 2, "block": "p", "category": "noble gas", "mp_k": 24.56, "bp_k": 27.07, "density": 0.0008999, "en": None},
        {"z": 11, "symbol": "Na", "name": "Sodium", "mass": 22.990, "group": 1, "period": 3, "block": "s", "category": "alkali metal", "mp_k": 370.87, "bp_k": 1156, "density": 0.971, "en": 0.93},
        {"z": 12, "symbol": "Mg", "name": "Magnesium", "mass": 24.305, "group": 2, "period": 3, "block": "s", "category": "alkaline earth metal", "mp_k": 923, "bp_k": 1363, "density": 1.738, "en": 1.31},
        {"z": 13, "symbol": "Al", "name": "Aluminum", "mass": 26.982, "group": 13, "period": 3, "block": "p", "category": "post-transition metal", "mp_k": 933.47, "bp_k": 2792, "density": 2.698, "en": 1.61},
        {"z": 14, "symbol": "Si", "name": "Silicon", "mass": 28.086, "group": 14, "period": 3, "block": "p", "category": "metalloid", "mp_k": 1687, "bp_k": 3538, "density": 2.3296, "en": 1.90},
        {"z": 15, "symbol": "P", "name": "Phosphorus", "mass": 30.974, "group": 15, "period": 3, "block": "p", "category": "reactive nonmetal", "mp_k": 317.30, "bp_k": 550, "density": 1.82, "en": 2.19},
        {"z": 16, "symbol": "S", "name": "Sulfur", "mass": 32.065, "group": 16, "period": 3, "block": "p", "category": "reactive nonmetal", "mp_k": 388.36, "bp_k": 717.87, "density": 2.067, "en": 2.58},
        {"z": 17, "symbol": "Cl", "name": "Chlorine", "mass": 35.453, "group": 17, "period": 3, "block": "p", "category": "reactive nonmetal", "mp_k": 171.6, "bp_k": 239.11, "density": 0.003214, "en": 3.16},
        {"z": 18, "symbol": "Ar", "name": "Argon", "mass": 39.948, "group": 18, "period": 3, "block": "p", "category": "noble gas", "mp_k": 83.80, "bp_k": 87.30, "density": 0.0017837, "en": None},
        {"z": 19, "symbol": "K", "name": "Potassium", "mass": 39.098, "group": 1, "period": 4, "block": "s", "category": "alkali metal", "mp_k": 336.53, "bp_k": 1032, "density": 0.862, "en": 0.82},
        {"z": 20, "symbol": "Ca", "name": "Calcium", "mass": 40.078, "group": 2, "period": 4, "block": "s", "category": "alkaline earth metal", "mp_k": 1115, "bp_k": 1757, "density": 1.54, "en": 1.00},
    ]
    
    return {"elements": sample_elements}

def emit_tables(elements_data, out_h):
    """Generate C header file with packed element data"""
    elements = elements_data['elements']
    
    # String tables
    sym_blob = bytearray()
    name_blob = bytearray()
    disc_blob = bytearray()
    
    # Packed element array
    packed = [None] * 119  # Index 0 unused, 1-118 for elements
    
    for elem in elements:
        z = elem['z']
        
        # Add strings to tables
        sym_off = add_string_to_table(sym_blob, elem['symbol'])
        name_off = add_string_to_table(name_blob, elem['name'])
        disc_off = add_string_to_table(disc_blob, elem.get('discoverer', ''))
        
        # Create packed record with proper type handling
        rec = OrderedDict([
            ('z', z),
            ('mass_milli', scale_value(elem.get('mass'), 1000)),
            ('mp_kx100', max(-32768, min(32767, scale_value(elem.get('mp_k'), 100)))),  # Clamp to int16_t range
            ('bp_kx100', max(-32768, min(32767, scale_value(elem.get('bp_k'), 100)))),  # Clamp to int16_t range
            ('density_mgx1000', max(0, min(65535, scale_value(elem.get('density'), 1000, 0)))),  # Clamp to uint16_t range
            ('ion_eVx1000', max(0, min(65535, scale_value(elem.get('ionization_ev'), 1000, 0)))),  # Clamp to uint16_t range
            ('en_paulingx100', max(0, min(65535, scale_value(elem.get('en'), 100, 0)))),  # Clamp to uint16_t range
            
            ('group', elem.get('group', 0)),
            ('period', elem.get('period', 0)),
            ('valence_e', elem.get('valence_e', 0)),
            ('oxidation_min', (elem.get('oxid_min', 0) + 64) & 0xFF),
            ('oxidation_max', (elem.get('oxid_max', 0) + 64) & 0xFF),
            ('flags', elem.get('flags', 0)),
            ('category', f'static_cast<ElemCategory>({map_category(elem.get("category", "unknown"))})'),
            ('block', f'static_cast<ElemBlock>({map_block(elem.get("block", "s"))})'),
            
            ('sym_off', sym_off),
            ('name_off', name_off),
            ('discoverer_off', disc_off),
            ('discovery_year', elem.get('discovery_year', 0))
        ])
        
        packed[z] = rec
    
    # Write header file
    with open(out_h, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include "periodic_data.h"\n\n')
        
        # String tables
        def write_blob(name, blob):
            f.write(f'const uint8_t {name}[] = {{')
            if blob:
                f.write(','.join(str(b) for b in blob))
            f.write('};\n')
            size_name = name + '_SIZE'
            f.write(f'const uint32_t {size_name} = {len(blob)};\n\n')
        
        write_blob('PT_SYM_BYTES', sym_blob)
        write_blob('PT_NAME_BYTES', name_blob)
        write_blob('PT_DISC_BYTES', disc_blob)
        
        # Element table
        f.write('const PackedElement PT_ELEMENTS[119] = {\n')
        f.write('  {0}, // index 0 unused\n')
        
        for z in range(1, 119):
            if packed[z]:
                r = packed[z]
                f.write('  {')
                # Handle special formatting for enum values
                values = []
                for k in r:
                    val = r[k]
                    if isinstance(val, str) and 'static_cast' in val:
                        values.append(val)  # Already formatted enum cast
                    else:
                        values.append(str(val))
                f.write(','.join(values))
                f.write('},\n')
            else:
                f.write('  {0}, // element not defined\n')
        
        f.write('};\n')

def main():
    if len(sys.argv) < 3:
        print("Usage: gen_elements.py <input.json> <output.h>")
        print("       gen_elements.py --sample <output.h>  (generate sample data)")
        sys.exit(1)
    
    if sys.argv[1] == '--sample':
        print("Generating sample data for first 20 elements...")
        elements_data = generate_sample_data()
        emit_tables(elements_data, sys.argv[2])
        print(f"Generated {sys.argv[2]} with sample element data")
    else:
        print(f"Reading element data from {sys.argv[1]}...")
        with open(sys.argv[1], 'r') as f:
            elements_data = json.load(f)
        emit_tables(elements_data, sys.argv[2])
        print(f"Generated {sys.argv[2]} with {len(elements_data['elements'])} elements")

if __name__ == '__main__':
    main()
