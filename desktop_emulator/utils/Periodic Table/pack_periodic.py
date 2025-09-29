#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
pack_periodic.py
Read Bowserinator/Periodic-Table-JSON -> emit periodic_data_pack.h for PocketMage.

Usage:
  python3 tools/pack_periodic.py /path/to/PeriodicTableJSON.json src/periodic_data_pack.h

Repo fields (examples) from README (Hydrogen):
- number, symbol, name, atomic_mass, boil (K), melt (K), density (g/L or g/cm3),
  period, group, block, phase ("Gas","Solid","Liquid"), category ("... metal", "metalloid", "nonmetal"),
  electron_configuration, electron_configuration_semantic,
  electronegativity_pauling, electron_affinity, ionization_energies[], discovered_by, year_discovered (sometimes),
  image/spectral, shells[], xpos/ypos, summary, source, etc.
See: https://github.com/Bowserinator/Periodic-Table-JSON
"""
import json, sys, math
from collections import defaultdict

OUT_GUARD = "PERIODIC_DATA_PACK_H"

def die(msg):
    print(f"ERROR: {msg}", file=sys.stderr); sys.exit(1)

def rd(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

# ---- Enum mappers (dataset strings -> your enums) ----

def map_category(cat: str):
    if not cat: return "ElemCategory::Unknown"
    c = cat.strip().lower()
    if "alkali metal" in c:             return "ElemCategory::AlkaliMetal"
    if "alkaline earth metal" in c:     return "ElemCategory::AlkalineEarth"
    if "transition metal" in c:         return "ElemCategory::Transition"
    if "post-transition metal" in c:    return "ElemCategory::PostTransition"
    if "metalloid" in c:                return "ElemCategory::Metalloid"
    if "noble gas" in c:                return "ElemCategory::NobleGas"
    if "lanthan" in c:                  return "ElemCategory::Lanthanoid"
    if "actin" in c:                    return "ElemCategory::Actinoid"
    # nonmetals come in flavors: "diatomic nonmetal", "polyatomic nonmetal", sometimes plain "nonmetal"
    if "nonmetal" in c:                 return "ElemCategory::ReactiveNonmetal"
    return "ElemCategory::Unknown"

def map_block(b):
    if not b: return "ElemBlock::Unknown"
    b = b.strip().lower()
    if b == "s": return "ElemBlock::s"
    if b == "p": return "ElemBlock::p"
    if b == "d": return "ElemBlock::d"
    if b == "f": return "ElemBlock::f"
    return "ElemBlock::Unknown"

def map_phase(p):
    if not p: return "ElemPhase::Unknown"
    p = p.strip().lower()
    if p == "solid":  return "ElemPhase::Solid"
    if p == "liquid": return "ElemPhase::Liquid"
    if p == "gas":    return "ElemPhase::Gas"
    return "ElemPhase::Unknown"

# Best effort valence electrons from shells (outermost shell count)
def valence_from_shells(shells):
    try:
        if not shells: return 0
        return int(shells[-1]) if len(shells) > 0 else 0
    except Exception:
        return 0

# Extract oxidation min/max from textual oxidation states if present (e.g., "−3, −2, −1, +1, +2, +3")
def oxid_range(ox_field):
    if not ox_field: return (None, None)
    # common in some datasets; Bowserinator JSON may or may not have 'oxidation_states'
    s = str(ox_field)
    vals = []
    tok = s.replace("−","-").replace("—","-").replace("–","-").replace("+"," +").replace(","," ").split()
    for t in tok:
        try:
            if t in {"+","-"}: continue
            vals.append(int(t))
        except:
            pass
    if not vals: return (None, None)
    return (min(vals), max(vals))

def scale_or(val, scale, none=0, signed=False):
    if val is None: return none
    try:
        x = float(val)
    except:
        return none
    v = int(round(x * scale))
    if signed:
        # clamp into int16
        if v < -32768: v = -32768
        if v >  32767: v =  32767
    else:
        if v < 0: v = 0
        if v > 65535: v = 65535
    return v

def year_from_element(e):
    # Prefer 'year_discovered' if present, else try to parse from e['discovered_by'] or leave 0
    y = e.get("year_discovered")
    if y is None:
        return 0
    try:
        ys = str(y).strip()
        if not ys: return 0
        # handle BCE like "-700"
        return int(ys)
    except:
        return 0

def first_ionization_eV(e):
    arr = e.get("ionization_energies")
    if not isinstance(arr, list) or not arr: return None
    # In the dataset ionization energies are in kJ/mol; convert to eV (1 eV = 96.485 kJ/mol)
    # README says "Ionization energies are given as an array" (units implied kJ/mol). Convert to eV.
    try:
        kJmol = float(arr[0])
        eV = kJmol / 96.485
        return eV
    except:
        return None

def electron_affinity_eV(e):
    # README: "electron_affinity" is the energy required to detach an electron from the anion (units: likely kJ/mol)
    # Convert kJ/mol -> eV (same factor).
    ea = e.get("electron_affinity")
    if ea is None: return None
    try:
        kJmol = float(ea)
        return kJmol / 96.485
    except:
        return None

def main(json_path, out_path):
    root = rd(json_path)
    elements = root["elements"]
    # Prepare string blobs (symbols, names, discoverers)
    sym_blob = bytearray()
    name_blob = bytearray()
    disc_blob = bytearray()

    def add_str(blob, s):
        off = len(blob); blob += s.encode("utf-8") + b"\x00"; return off

    # Index by atomic number
    packed = [None] * 119
    seen_z = set()

    for e in elements:
        z = int(e["number"])
        if z < 1 or z > 118: continue
        if z in seen_z: die(f"Duplicate atomic number {z}")
        seen_z.add(z)

        sym_off  = add_str(sym_blob, e.get("symbol",""))
        name_off = add_str(name_blob, e.get("name",""))
        disc     = e.get("discovered_by") or ""
        discoverer_off = add_str(disc_blob, disc)

        # units: K for melt/boil, g/cm3 for solids/liquids and g/L for gases, EN Pauling, mass in atomic mass units
        mass_milli  = scale_or(e.get("atomic_mass"), 1000, none=0)
        mp_kx100    = scale_or(e.get("melt"), 100, none=-1, signed=True)
        bp_kx100    = scale_or(e.get("boil"), 100, none=-1, signed=True)

        dens = e.get("density")
        density_x1000 = scale_or(dens, 1000, none=0)

        en  = e.get("electronegativity_pauling")
        en_paulingx100 = scale_or(en, 100, none=0)

        ion1_eV = first_ionization_eV(e)
        ion_eVx1000 = scale_or(ion1_eV, 1000, none=0)

        ea_eV = electron_affinity_eV(e)
        electron_aff_x100 = scale_or(ea_eV, 100, none=0)

        group  = int(e.get("group") or 0)
        period = int(e.get("period") or 0)
        block  = map_block(e.get("block"))
        category = map_category(e.get("category"))

        shells = e.get("shells") or []
        valence_e = valence_from_shells(shells)

        oxid_min, oxid_max = oxid_range(e.get("oxidation_states"))
        if oxid_min is None:
            oxid_min_b = 127  # sentinel = unknown
            oxid_max_b = 127
        else:
            # bias by +64 into int8_t
            oxid_min_b = max(-64, min(63, oxid_min)) + 64
            oxid_max_b = max(-64, min(63, oxid_max)) + 64

        flags = 0
        # If you later curate toxicity/radioactivity, set bits here (left as 0 for now)

        discovery_year = year_from_element(e)

        packed[z] = dict(
            z=z,
            mass_milli=mass_milli,
            mp_kx100=mp_kx100,
            bp_kx100=bp_kx100,
            density_x1000=density_x1000,
            ion_eVx1000=ion_eVx1000,
            en_paulingx100=en_paulingx100,
            electron_aff_x100=electron_aff_x100,
            group=group,
            period=period,
            valence_e=valence_e,
            oxid_min_biased=oxid_min_b,
            oxid_max_biased=oxid_max_b,
            flags=flags,
            category=category,
            block=block,
            sym_off=sym_off,
            name_off=name_off,
            discoverer_off=discoverer_off,
            discovery_year=discovery_year,
        )

    # Emit header
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("// AUTO-GENERATED by pack_periodic.py — do not edit.\n")
        f.write("#pragma once\n")
        f.write('#include "periodic_data.h"\n\n')

        # blobs
        f.write(f"const uint8_t PT_SYM_BYTES[] = {{{','.join(map(str, sym_blob))}}};\n")
        f.write(f"const uint32_t PT_SYM_SIZE = {len(sym_blob)};\n")
        f.write("\n")
        f.write(f"const uint8_t PT_NAME_BYTES[] = {{{','.join(map(str, name_blob))}}};\n")
        f.write(f"const uint32_t PT_NAME_SIZE = {len(name_blob)};\n")
        f.write("\n")
        f.write(f"const uint8_t PT_DISC_BYTES[] = {{{','.join(map(str, disc_blob))}}};\n")
        f.write(f"const uint32_t PT_DISC_SIZE = {len(disc_blob)};\n")

        # table
        f.write("const PackedElement PT_ELEMENTS[119] = {\n")
        f.write("  {0}, // index 0 unused\n")
        for z in range(1,119):
            r = packed[z]
            if r is None:
                # fill hole if any (should not happen)
                f.write("  {0},\n")
                continue
            f.write("  {")
            f.write(f"{r['z']},")
            f.write(f"{r['mass_milli']},")
            f.write(f"{r['mp_kx100']},")
            f.write(f"{r['bp_kx100']},")
            f.write(f"{r['density_x1000']},")
            f.write(f"{r['ion_eVx1000']},")
            f.write(f"{r['en_paulingx100']},")
            f.write(f"{r['electron_aff_x100']},")
            f.write(f"{r['group']},")
            f.write(f"{r['period']},")
            f.write(f"{r['valence_e']},")
            f.write(f"{r['oxid_min_biased']},")
            f.write(f"{r['oxid_max_biased']},")
            f.write(f"{r['flags']},")
            f.write(f"{r['category']},")
            f.write(f"{r['block']},")
            f.write(f"{r['sym_off']},")
            f.write(f"{r['name_off']},")
            f.write(f"{r['discoverer_off']},")
            f.write(f"{r['discovery_year']}")
            f.write("},\n")
        f.write("};\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        die("Usage: pack_periodic.py PeriodicTableJSON.json src/periodic_data_pack.h")
    main(sys.argv[1], sys.argv[2])
