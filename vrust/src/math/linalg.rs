/// Simple power iteration to approximate spectral radius (dominant eigenvalue)
/// of a non-negative square matrix A given as Vec<Vec<f64>>.
pub fn spectral_radius_power_iteration(a: &[Vec<f64>], max_iter: usize, tol: f64) -> f64 {
    let n = a.len();
    assert!(n > 0 && a.iter().all(|row| row.len() == n), "Matrix must be square");

    let mut x = vec![1.0 / (n as f64); n];
    let mut lambda_old = 0.0;

    for _ in 0..max_iter {
        // y = A x
        let mut y = vec![0.0; n];
        for i in 0..n {
            let mut sum = 0.0;
            for j in 0..n {
                sum += a[i][j] * x[j];
            }
            y[i] = sum;
        }
        // Rayleigh quotient approx
        let mut num = 0.0;
        let mut den = 0.0;
        for i in 0..n {
            num += y[i] * x[i];
            den += x[i] * x[i];
        }
        let lambda = if den > 0.0 { num / den } else { 0.0 };

        // normalize y -> x
        let norm = y.iter().map(|v| v * v).sum::<f64>().sqrt();
        if norm > 0.0 {
            for i in 0..n { x[i] = y[i] / norm; }
        }
        if (lambda - lambda_old).abs() < tol {
            return lambda;
        }
        lambda_old = lambda;
    }
    lambda_old
}
