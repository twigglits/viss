/// Simple fixed-step RK4 integrator for systems of ODEs.
/// State and derivative are represented as Vec<f64>.
pub fn rk4_step<F>(y: &mut [f64], t: f64, dt: f64, mut f: F)
where
    F: FnMut(f64, &[f64], &mut [f64]),
{
    let n = y.len();
    let mut k1 = vec![0.0; n];
    let mut k2 = vec![0.0; n];
    let mut k3 = vec![0.0; n];
    let mut k4 = vec![0.0; n];
    let mut ytmp = vec![0.0; n];

    f(t, y, &mut k1);

    for i in 0..n {
        ytmp[i] = y[i] + 0.5 * dt * k1[i];
    }
    f(t + 0.5 * dt, &ytmp, &mut k2);

    for i in 0..n {
        ytmp[i] = y[i] + 0.5 * dt * k2[i];
    }
    f(t + 0.5 * dt, &ytmp, &mut k3);

    for i in 0..n {
        ytmp[i] = y[i] + dt * k3[i];
    }
    f(t + dt, &ytmp, &mut k4);

    for i in 0..n {
        y[i] += (dt / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
    }
}

/// Workspace for allocation-free RK4 steps
pub struct Rk4Workspace {
    pub k1: Vec<f64>,
    pub k2: Vec<f64>,
    pub k3: Vec<f64>,
    pub k4: Vec<f64>,
    pub ytmp: Vec<f64>,
}

impl Rk4Workspace {
    pub fn new(n: usize) -> Self {
        Self {
            k1: vec![0.0; n],
            k2: vec![0.0; n],
            k3: vec![0.0; n],
            k4: vec![0.0; n],
            ytmp: vec![0.0; n],
        }
    }

    pub fn resize(&mut self, n: usize) {
        if self.k1.len() != n {
            self.k1.resize(n, 0.0);
            self.k2.resize(n, 0.0);
            self.k3.resize(n, 0.0);
            self.k4.resize(n, 0.0);
            self.ytmp.resize(n, 0.0);
        }
    }
}

/// Fixed-step RK4 using preallocated workspace to avoid allocations per step.
pub fn rk4_step_ws<F>(y: &mut [f64], t: f64, dt: f64, ws: &mut Rk4Workspace, mut f: F)
where
    F: FnMut(f64, &[f64], &mut [f64]),
{
    let n = y.len();
    ws.resize(n);

    let (k1, k2, k3, k4, ytmp) = (&mut ws.k1, &mut ws.k2, &mut ws.k3, &mut ws.k4, &mut ws.ytmp);

    f(t, y, k1);

    for i in 0..n {
        ytmp[i] = y[i] + 0.5 * dt * k1[i];
    }
    f(t + 0.5 * dt, ytmp, k2);

    for i in 0..n {
        ytmp[i] = y[i] + 0.5 * dt * k2[i];
    }
    f(t + 0.5 * dt, ytmp, k3);

    for i in 0..n {
        ytmp[i] = y[i] + dt * k3[i];
    }
    f(t + dt, ytmp, k4);

    for i in 0..n {
        y[i] += (dt / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
    }
}
