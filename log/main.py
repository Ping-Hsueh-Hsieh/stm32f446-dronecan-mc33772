import pandas as pd
from pathlib import Path
import numpy as np

LOG_FILE = Path("./log.csv")

uint32_col  = ['timestamp_ms']
float32_cols = ['current_A', 'stack_V',
                'cell00_V','cell01_V','cell02_V',
                'cell03_V','cell04_V','cell05_V']

if __name__ == '__main__':
    if not LOG_FILE.is_file():
        print(f"[ERROR] `{LOG_FILE.as_posix()}` not found")
        exit(1)

    df = pd.read_csv(LOG_FILE)

    # Parse hex strings → uint32, then reinterpret bits as float32
    for col in float32_cols:
        uints = np.array([int(x, 16) for x in df[col]], dtype=np.uint32)
        df[col] = uints.view(np.float32)   # reinterpret same bits as float32

    df['timestamp_ms'] = df['timestamp_ms'].apply(lambda x: int(x, 16))

    print(df)

