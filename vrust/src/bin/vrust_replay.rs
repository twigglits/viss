//! Replay a VISS run config downloaded from a hosted instance.
//!
//!     cargo run --bin vrust_replay -- input.json > output.csv
//!     curl .../run_config/<run_id> | cargo run --bin vrust_replay > output.csv
//!
//! Writes the same CSV the hosted instance serves from `/run_output/<run_id>`, so the two
//! can be diffed directly. No database, no network — the config carries every input.

use std::io::Read;

use vrust::io::run_config::RunConfig;

fn main() -> anyhow::Result<()> {
    let path = std::env::args().nth(1);
    let mut raw = String::new();
    match path.as_deref() {
        None | Some("-") => {
            std::io::stdin().read_to_string(&mut raw)?;
        }
        Some(p) => raw = std::fs::read_to_string(p)?,
    }

    let cfg: RunConfig = serde_json::from_str(&raw)?;
    let traj = cfg.simulate()?;
    eprintln!(
        "[vrust-replay] {} model, {} {}, {} days at dt={} — {} grid points",
        cfg.model, cfg.iso3, cfg.year, cfg.t_end_days, cfg.dt_days, traj.len()
    );
    print!("{}", cfg.to_csv(&traj));
    Ok(())
}
