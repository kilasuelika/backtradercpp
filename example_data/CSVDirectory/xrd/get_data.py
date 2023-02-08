import akshare as ak
import glob
import time

codes=[s[-13:-4] for s in glob.glob("../raw/*")]
codes

for cd in codes:
    df = ak.stock_history_dividend_detail(symbol=cd[:6], indicator="分红")
    df.to_csv(f"{cd}.csv")
    
    time.sleep(5)