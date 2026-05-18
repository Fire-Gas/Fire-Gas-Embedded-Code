#!/usr/bin/env python3
"""
normalizer.npz → normalizer_params.h 변환 스크립트.
빌드 전 반드시 1회 실행할 것:
    python gen_normalizer_header.py
"""
from pathlib import Path
import json
import numpy as np

ROOT   = Path(__file__).parent
NPZ    = ROOT / "ESP32_deploy" / "params" / "normalizer.npz"
COLS_F = ROOT / "ESP32_deploy" / "params" / "feature_columns.json"
OUT    = ROOT / "normalizer_params.h"

npz  = np.load(NPZ)
mean = npz["mean"].astype(np.float32)
std  = npz["std"].astype(np.float32)
cols = json.loads(COLS_F.read_text())

assert len(mean) == len(std) == len(cols), \
    f"크기 불일치: mean={len(mean)}, std={len(std)}, cols={len(cols)}"

lines = [
    "#pragma once",
    "/* !! 자동 생성 파일 — gen_normalizer_header.py 로 재생성하세요 !! */",
    f"#define NORM_FEATURE_COUNT {len(cols)}",
    "",
    "static const float NORM_MEAN[NORM_FEATURE_COUNT] = {",
]
for i, (name, val) in enumerate(zip(cols, mean)):
    comma = "," if i < len(cols) - 1 else ""
    lines.append(f"    {val:.8f}f{comma}  /* {name} */")
lines += ["};", "", "static const float NORM_STD[NORM_FEATURE_COUNT] = {"]
for i, (name, val) in enumerate(zip(cols, std)):
    comma = "," if i < len(cols) - 1 else ""
    safe = float(max(abs(val), 1e-6))
    lines.append(f"    {safe:.8f}f{comma}  /* {name} */")
lines += ["};", ""]

OUT.write_text("\n".join(lines))
print(f"생성 완료: {OUT}  ({len(cols)} features)")
