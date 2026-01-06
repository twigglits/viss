/// Synthetic contact matrix generator for testing.
///
/// This is NOT meant to be realistic, only to unblock end-to-end simulations while
/// a country-specific contact matrix is sourced.
///
/// The returned matrix is symmetric with higher weights on same/nearby age bins.

pub fn synthetic_contact_matrix(n_age: usize) -> Vec<Vec<f64>> {
    let mut c = vec![vec![0.0; n_age]; n_age];
    for i in 0..n_age {
        for j in 0..n_age {
            let d = (i as i32 - j as i32).abs() as f64;
            // strong diagonal + exponential decay with distance
            let v = 10.0 * (-0.7 * d).exp() + 0.5;
            c[i][j] = v;
        }
    }
    c
}
