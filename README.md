Collection of projects demonstrating basic methods in machine learning, data science, data pre-processing, and analytics.

---

## Psychology of Human Misjudgment
This project demonstrates psychological tendencies described in Poor Charlie's Almanack, using real-world industry datasets and machine learning.

### Features
- Reward and Punishment Superresponse Tendency
  - Demonstrated with insurance industry data comparing National Indemnity (Berkshire Hathaway) vs. industry averages
  - Applied **KMeans clustering** to premium volume changes and underwriting profits, highlighting how incentive structures shaped distinct outcomes
  - Validated cluster separation with a confusion matrix
- Doubt-Avoidance Tendency
  - Illustrated with credit card industry data (American Express, Visa, Mastercard)
  - Applied **linear regression** to examine the relationship between cards-in-force, transaction volumes, and pretax income
  - Showed how common assumptions about Amex's superior scaling (due to closed-loop network) are false
- Data Processing & Feature Engineering
  - Merged CSV datasets with pandas for insurance and credit card companies
  - Engineered features such as percentage growth rates
  - Excluded anomalous years (e.g., 2020-2021) to ensure validity of illustrations
- Visualizations
  - Scatterplots and regression lines showing credit card growth vs. profitability
  - KMeans clustering of insurance profitability patterns
  - Confusion matrix contrasting predicted vs. true labels (NICO vs Industry)
  - Comparative line charts of underwriting profits and premium growth

### Repository Structure

`psychology_of_misjudgment.ipynb`
- Notebook demonstrating reward/punishment and doubt-avoidance tendencies with real-world data analysis and machine learning
`data/`
- `insurance_nico.csv`, `insurance_industry.csv` - premium volumes and underwriting profits
- `american_express.csv`, `visa.csv`, `mastercard.csv` - cards-in-force, transaction volumes, pretax income

### Technical Skills
- Python: pandas, NumPy, matplotlib, seaborn
- Machine Learning: scikit-learn (Kmeans, Linear Regression, Confusion Matrix)

---
## Financial Data Pre-Processing and Analytics
This project implements a data pipeline for retrieving, cleaning, and analyzing financial datasets of S&P 500 constituents. It demonstrates retrieval of data from APIs, basic data transformation into structured datasets, derived metrics, and visualizations. 

### Features
- Automated data retrieval of S&P 500 constituents
- Data pre-processing and cleaning: renames columns using JSON mapping, parses company headquarters into U.S. state names, converts date fields, computes annual returns, imputes missing growth rates, derives additional metrics, and produces merged datasets with 100+ features
- Visualization: sector composition across major U.S. states, heatmap of S&P 500 additions by decade and headquarters location, annual returns of S&P 500 vs 1Y Treasury yields, cumulative growth trends of net income, free cash flow, and index returns
- Applies a common-sense investment checklist (e.g., cash > long-term debt, earnings & cash flow growth > 15%, pe < 50) on 2001-2010 averages, evaluating returns for 2011-2020 to demonstrate how a simple common-sense checklist can be useful

### Repository Structure

`data_preprocessing_analytics_part1.ipynb`
- Demonstrates financial data retrieval, cleaning, and initial exploratory analysis
`data_preprocessing_analytics_part2.ipynb`
- Merges datasets, handles missing values, and applies an investment checklist
`data/`
- Contains processed CSV files such as:
  - `sp500_constituents.csv`
  - `income_statement.csv`, `balance_sheet.csv`, etc.
  - `index_data.csv` and `index_1yr_returns.csv`
  - `merged_financial_data.csv`
`renames.json`
- JSON mapping for renaming columns consistently across datasets

### Technical Skills
- Python: pandas, NumPy, requests
- Visualization: matplotlib, seaborn
- Environment Management: python-dotenv for secure API key handling
