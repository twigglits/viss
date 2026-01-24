use anyhow::Context;

pub fn write_seirs_debug_log(
    out_dir: impl AsRef<std::path::Path>,
    run_id: &str,
    iso3: &str,
    year: i32,
    seed_infections: f64,
    t_end: f64,
    dt: f64,
    population: &[(f64, f64)],
    infected: &[(f64, f64)],
    incidence: &[(f64, f64)],
) -> anyhow::Result<std::path::PathBuf> {
    use std::io::Write;

    std::fs::create_dir_all(out_dir.as_ref()).context("create logs dir failed")?;
    let path = out_dir.as_ref().join(format!("seirs_{}.txt", run_id));
    let mut f = std::fs::File::create(&path)
        .with_context(|| format!("create debug log file failed (path={:?})", path))?;

    writeln!(f, "run_id={}", run_id)?;
    writeln!(f, "iso3={}", iso3)?;
    writeln!(f, "year={}", year)?;
    writeln!(f, "seed_infections={:.6}", seed_infections)?;
    writeln!(f, "t_end_days={:.6}", t_end)?;
    writeln!(f, "dt_days={:.6}", dt)?;
    writeln!(f, "")?;
    writeln!(f, "t,population,infected,incidence_pct")?;

    for (((t1, p), (t2, i)), (t3, inc)) in population.iter().zip(infected.iter()).zip(incidence.iter()) {
        anyhow::ensure!((t1 - t2).abs() < 1e-9, "timeline t mismatch");
        anyhow::ensure!((t1 - t3).abs() < 1e-9, "timeline t mismatch");
        writeln!(f, "{:.6},{:.0},{:.0},{:.6}", t1, p, i, inc)?;
    }

    Ok(path)
}
