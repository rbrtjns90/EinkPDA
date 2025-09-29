#!/usr/bin/env python3
"""
Pokemon Data Converter for PocketMage Pokedex App

Converts scraped Pokemon JSON data into binary files expected by the PocketMage Pokedex app:
- pokemon_names.str: String table with Pokemon names
- pokemon_data.rec: Binary records with Pokemon stats and info
- pokemon_index.idx: Index file for fast lookups
- pokemon_sprites.bin: Compressed sprite data

Usage: python pokemon_data_converter.py
"""

import json
import struct
import os
from pathlib import Path
from PIL import Image
import io

def load_pokemon_data():
    """Load the simplified Pokemon JSON data."""
    data_file = Path("pokemon_data/data/pokemon_simplified.json")
    if not data_file.exists():
        raise FileNotFoundError(f"Pokemon data file not found: {data_file}")
    
    with open(data_file, 'r') as f:
        return json.load(f)

def create_string_table(pokemon_list):
    """Create a string table (.str) file with Pokemon names."""
    output_file = Path("pokemon_data/pokemon_names.str")
    
    # String table format: [count][offset1][offset2]...[string1][string2]...
    string_data = b""
    offsets = []
    
    for pokemon in pokemon_list:
        offsets.append(len(string_data))
        name_bytes = pokemon['name'].encode('utf-8') + b'\0'  # null-terminated
        string_data += name_bytes
    
    # Write header: count (2 bytes) + offsets (2 bytes each)
    with open(output_file, 'wb') as f:
        f.write(struct.pack('<H', len(pokemon_list)))  # count
        for offset in offsets:
            f.write(struct.pack('<H', offset))  # offset
        f.write(string_data)  # strings
    
    print(f"Created string table: {output_file} ({len(pokemon_list)} names)")
    return output_file

def create_pokemon_records(pokemon_list):
    """Create binary Pokemon records (.rec) file."""
    output_file = Path("pokemon_data/pokemon_data.rec")
    
    # Record format (32 bytes per Pokemon):
    # id (2), height (2), weight (2), hp (1), attack (1), defense (1), 
    # sp_attack (1), sp_defense (1), speed (1), type1 (1), type2 (1),
    # genus_offset (2), flavor_offset (2), sprite_offset (4), padding (10)
    
    # Build type mapping
    type_map = {
        'normal': 1, 'fire': 2, 'water': 3, 'electric': 4, 'grass': 5,
        'ice': 6, 'fighting': 7, 'poison': 8, 'ground': 9, 'flying': 10,
        'psychic': 11, 'bug': 12, 'rock': 13, 'ghost': 14, 'dragon': 15,
        'dark': 16, 'steel': 17, 'fairy': 18
    }
    
    # Collect all genus and flavor text for string table
    genus_strings = []
    flavor_strings = []
    genus_offsets = {}
    flavor_offsets = {}
    
    for pokemon in pokemon_list:
        genus = pokemon.get('genus', 'Unknown')
        flavor = pokemon.get('flavor_text', 'No description available.')
        
        if genus not in genus_offsets:
            genus_offsets[genus] = len(genus_strings)
            genus_strings.append(genus)
        
        if flavor not in flavor_offsets:
            flavor_offsets[flavor] = len(flavor_strings)
            flavor_strings.append(flavor)
    
    with open(output_file, 'wb') as f:
        for i, pokemon in enumerate(pokemon_list):
            # Basic data
            poke_id = pokemon['id']
            height = pokemon.get('height', 0)
            weight = pokemon.get('weight', 0)
            
            # Stats
            stats = pokemon.get('stats', {})
            hp = min(255, stats.get('hp', 0))
            attack = min(255, stats.get('attack', 0))
            defense = min(255, stats.get('defense', 0))
            sp_attack = min(255, stats.get('special_attack', 0))
            sp_defense = min(255, stats.get('special_defense', 0))
            speed = min(255, stats.get('speed', 0))
            
            # Types
            types = pokemon.get('types', [])
            type1 = type_map.get(types[0] if types else 'normal', 1)
            type2 = type_map.get(types[1] if len(types) > 1 else '', 0)
            
            # String offsets
            genus = pokemon.get('genus', 'Unknown')
            flavor = pokemon.get('flavor_text', 'No description available.')
            genus_offset = genus_offsets[genus]
            flavor_offset = flavor_offsets[flavor]
            
            # Sprite offset (will be calculated later)
            sprite_offset = i * 1024  # placeholder
            
            # Pack record (32 bytes)
            record = struct.pack('<HHHBBBBBBBBHHLxxxxxxxxxx',
                poke_id, height, weight, hp, attack, defense,
                sp_attack, sp_defense, speed, type1, type2,
                genus_offset, flavor_offset, sprite_offset)
            
            f.write(record)
    
    # Write genus strings
    genus_file = Path("pokemon_data/pokemon_genus.str")
    with open(genus_file, 'wb') as f:
        f.write(struct.pack('<H', len(genus_strings)))
        offset = 0
        for genus in genus_strings:
            f.write(struct.pack('<H', offset))
            offset += len(genus.encode('utf-8')) + 1
        for genus in genus_strings:
            f.write(genus.encode('utf-8') + b'\0')
    
    # Write flavor strings
    flavor_file = Path("pokemon_data/pokemon_flavor.str")
    with open(flavor_file, 'wb') as f:
        f.write(struct.pack('<H', len(flavor_strings)))
        offset = 0
        for flavor in flavor_strings:
            f.write(struct.pack('<H', offset))
            offset += len(flavor.encode('utf-8')) + 1
        for flavor in flavor_strings:
            f.write(flavor.encode('utf-8') + b'\0')
    
    print(f"Created Pokemon records: {output_file} ({len(pokemon_list)} records)")
    print(f"Created genus strings: {genus_file} ({len(genus_strings)} entries)")
    print(f"Created flavor strings: {flavor_file} ({len(flavor_strings)} entries)")
    
    return output_file

def create_index_file(pokemon_list):
    """Create index file (.idx) for fast Pokemon lookups."""
    output_file = Path("pokemon_data/pokemon_index.idx")
    
    # Index format: [count][id1][offset1][id2][offset2]...
    with open(output_file, 'wb') as f:
        f.write(struct.pack('<H', len(pokemon_list)))
        
        for i, pokemon in enumerate(pokemon_list):
            poke_id = pokemon['id']
            record_offset = i * 32  # 32 bytes per record
            f.write(struct.pack('<HL', poke_id, record_offset))
    
    print(f"Created index file: {output_file} ({len(pokemon_list)} entries)")
    return output_file

def convert_sprite_to_bitmap(image_path, target_width=64, target_height=64):
    """Convert PNG sprite to 1-bit bitmap data for E-Ink display."""
    try:
        with Image.open(image_path) as img:
            # Convert to grayscale and resize
            img = img.convert('L').resize((target_width, target_height), Image.Resampling.LANCZOS)
            
            # Convert to 1-bit using dithering
            img = img.convert('1', dither=Image.Dither.FLOYDSTEINBERG)
            
            # Convert to bitmap bytes (1 bit per pixel, packed)
            bitmap_data = b""
            pixels = list(img.getdata())
            
            for y in range(target_height):
                for x in range(0, target_width, 8):
                    byte_val = 0
                    for bit in range(8):
                        if x + bit < target_width:
                            pixel_idx = y * target_width + x + bit
                            if pixel_idx < len(pixels) and pixels[pixel_idx] == 0:  # black pixel
                                byte_val |= (1 << (7 - bit))
                    bitmap_data += struct.pack('B', byte_val)
            
            return bitmap_data
    except Exception as e:
        print(f"Error converting sprite {image_path}: {e}")
        # Return empty bitmap
        return b'\x00' * (target_width * target_height // 8)

def create_sprite_data(pokemon_list):
    """Create compressed sprite data (.bin) file."""
    output_file = Path("pokemon_data/pokemon_sprites.bin")
    images_dir = Path("pokemon_data/images")
    
    sprite_data = b""
    sprite_offsets = []
    
    for pokemon in pokemon_list:
        sprite_offsets.append(len(sprite_data))
        
        # Get front sprite filename
        front_image = pokemon.get('front_image', f"{pokemon['id']:03d}_front.png")
        image_path = images_dir / front_image
        
        # Convert sprite to bitmap
        bitmap = convert_sprite_to_bitmap(image_path)
        
        # Add bitmap size header (2 bytes) + bitmap data
        sprite_entry = struct.pack('<H', len(bitmap)) + bitmap
        sprite_data += sprite_entry
    
    # Write sprite data file
    with open(output_file, 'wb') as f:
        # Header: count + offsets
        f.write(struct.pack('<H', len(pokemon_list)))
        for offset in sprite_offsets:
            f.write(struct.pack('<L', offset))
        # Sprite data
        f.write(sprite_data)
    
    print(f"Created sprite data: {output_file} ({len(pokemon_list)} sprites, {len(sprite_data)} bytes)")
    return output_file

def main():
    """Main conversion process."""
    print("Pokemon Data Converter for PocketMage")
    print("=" * 40)
    
    try:
        # Load Pokemon data
        print("Loading Pokemon data...")
        pokemon_list = load_pokemon_data()
        print(f"Loaded {len(pokemon_list)} Pokemon")
        
        # Create output directory
        output_dir = Path("pokemon_data")
        output_dir.mkdir(exist_ok=True)
        
        # Generate binary files
        print("\nGenerating binary files...")
        create_string_table(pokemon_list)
        create_pokemon_records(pokemon_list)
        create_index_file(pokemon_list)
        create_sprite_data(pokemon_list)
        
        print("\nConversion complete!")
        print("\nGenerated files:")
        print("- pokemon_names.str: Pokemon names string table")
        print("- pokemon_data.rec: Pokemon stats and info records")
        print("- pokemon_genus.str: Pokemon genus descriptions")
        print("- pokemon_flavor.str: Pokemon flavor text")
        print("- pokemon_index.idx: Fast lookup index")
        print("- pokemon_sprites.bin: Compressed sprite data")
        
        print(f"\nCopy these files to your PocketMage SD card in the /pokemon/ directory")
        
    except Exception as e:
        print(f"Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
