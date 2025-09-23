use anyhow::Context;

/// Load a square contact matrix from CSV. Attempts to parse numeric cells; ignores
/// non-numeric headers if present. All rows must have the same number of numeric cells.
pub fn load_contact_matrix_csv(path: &str) -> anyhow::Result<Vec<Vec<f64>>> {
    let mut rdr = csv::ReaderBuilder::new()
        .has_headers(true)
        .from_path(path)
        .with_context(|| format!("Failed to open contact CSV: {}", path))?;

    let mut matrix: Vec<Vec<f64>> = Vec::new();
    for result in rdr.records() {
        let record = result?;
        let mut row_vals: Vec<f64> = Vec::new();
        for field in record.iter() {
            if let Ok(v) = field.trim().parse::<f64>() {
                row_vals.push(v);
            }
        }
        if !row_vals.is_empty() {
            matrix.push(row_vals);
        }
    }
    // Ensure square
    let n = matrix.len();
    anyhow::ensure!(n > 0, "contact matrix empty or unparsable");
    anyhow::ensure!(matrix.iter().all(|r| r.len() == n), "contact matrix must be square (n x n)");
    Ok(matrix)
}
