use anyhow::Context;
use postgres::{Client, NoTls};

pub const AGE_BINS_5YR: [&str; 17] = [
    "0-4",
    "5-9",
    "10-14",
    "15-19",
    "20-24",
    "25-29",
    "30-34",
    "35-39",
    "40-44",
    "45-49",
    "50-54",
    "55-59",
    "60-64",
    "65-69",
    "70-74",
    "75-79",
    "80+",
];

/// Load 5-year-binned age pyramid populations from Postgres table `age_pyramid_5yr`.
///
/// Returns (labels, pop_vector) where the ordering is always `AGE_BINS_5YR`.
///
/// `pg_conn_str` example:
/// "host=127.0.0.1 port=5432 user=airflow password=airflow dbname=viss"
pub fn load_age_pyramid_5yr_pg(
    pg_conn_str: &str,
    iso3: &str,
    year: i32,
) -> anyhow::Result<(Vec<String>, Vec<f64>)> {
    let mut client = Client::connect(pg_conn_str, NoTls)
        .with_context(|| "Failed to connect to Postgres")?;

    let rows = client
        .query(
            "SELECT age_bin, pop FROM age_pyramid_5yr WHERE iso3 = $1 AND year = $2",
            &[&iso3.to_uppercase(), &year],
        )
        .with_context(|| "Failed to query age_pyramid_5yr")?;

    let mut by_bin: std::collections::HashMap<String, f64> = std::collections::HashMap::new();
    for row in rows {
        let age_bin: String = row.get(0);
        let pop: f64 = row.get(1);
        by_bin.insert(age_bin, pop);
    }

    // Ensure required bins exist
    let mut labels: Vec<String> = Vec::with_capacity(AGE_BINS_5YR.len());
    let mut pops: Vec<f64> = Vec::with_capacity(AGE_BINS_5YR.len());
    for &b in AGE_BINS_5YR.iter() {
        let v = by_bin
            .get(b)
            .copied()
            .with_context(|| format!("Missing age_bin '{}' for iso3={} year={}", b, iso3, year))?;
        labels.push(b.to_string());
        pops.push(v.max(0.0));
    }

    Ok((labels, pops))
}
