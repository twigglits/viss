use anyhow::Context;
use serde::Deserialize;

#[derive(Debug, Deserialize)]
struct PopRow {
    age_group: String,
    pop: f64,
}

/// Load population by age from a CSV file with columns: `age_group,pop`.
/// Returns a Vec<f64> of populations in file order and the labels.
pub fn load_population_csv(path: &str) -> anyhow::Result<(Vec<String>, Vec<f64>)> {
    let mut rdr = csv::Reader::from_path(path)
        .with_context(|| format!("Failed to open population CSV: {}", path))?;
    let mut labels = Vec::new();
    let mut pops = Vec::new();
    for result in rdr.deserialize::<PopRow>() {
        let row = result?;
        labels.push(row.age_group);
        pops.push(row.pop.max(0.0));
    }
    Ok((labels, pops))
}
