A portfolio of data science and machine learning projects applied to behavioral and financial datasets, including psychology-based tendencies from *Poor Charlie’s Almanack* and large-scale S&P 500 data engineering.

**Tools & Libraries**: Python · Pandas · NumPy · scikit-learn · Matplotlib · Seaborn · Jupyter · Requests · Dotenv

---

## Psychology of Human Misjudgment
File: `psychology_of_misjudgment.ipynb`

Applies data processing and machine learning methods to illustrate psychology-based tendencies from *Poor Charlie's Almanack*:

- Reward and Punishment Superresponse Tendency 
  - Insurance data: compared National Indemnity vs. industry averages  
  - KMeans clustering on premium volume changes and underwriting profits — showed how incentive structures led to distinct outcomes  

- Doubt-Avoidance Tendency
  - Credit card industry data (Amex, Visa, Mastercard)  
  - Linear regression on cards-in-force, transaction volume, and pretax income — revealed flaws in common performance assumptions  

---

## Financial Data Processing and Analytics
Files:  
- `data_processing_analytics_part1.ipynb`  
- `data_processing_analytics_part2.ipynb`

Demonstrates financial data retrieval, preprocessing, and large-scale analytics using S&P 500 constituent data:

- Automated retrieval, cleaning, and visualization of company fundamentals  
- Engineered datasets with 100+ features for cross-sectional and time-series analysis  
- Applied a simple investment checklist (cash > debt, growth > 15%, PE < 50)  
  - Backtested 2001–2010 averages — evaluated 2011–2020 performance  
