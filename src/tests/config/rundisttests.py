#!/usr/bin/env python

import os
import sys
sys.path.append(os.path.realpath(os.path.join(os.path.realpath(__file__),"../../../../util")))

import histogram
import subprocess

settings = [ 
    { 
        "type": "fixed", 
        "params": [ ("value", 0.5) ],
        "plotrange": (-1, 1),
        "f": "samples=100000;s=0.01;f(x,p) = 1.0/(sqrt(2.0*pi)*s)*exp(-(x-p)**2/(2*s**2))"
    },
    {
        "type": "uniform",
        "params": [ ("min", -1), ("max", 4) ],
        "plotrange": (-2, 5),
        "f": "H(x) = (x/abs(x)+1.0)/2.0 ; f(x,a,b) = 1.0/(b-a)*H(x-a)*H(b-x)"
    },
    {   
        "type": "beta",
        "params": [ ("a", 2), ("b", 5), ("min", -2), ("max", 3) ],
        "plotrange": (-3,4),
        "f": "H(x) = (x/abs(x)+1.0)/2.0; f(x,a,b,k,l) = gamma(a+b)/(gamma(a)*gamma(b))*((x-k)/(l-k))**(a-1.0)*(1.0-((x-k)/(l-k)))**(b-1.0)*H(x-k)*H(l-x)/(l-k)"
    },
    {   
        "type": "gamma",
        "params": [ ("a", 5), ("b", 1.5) ],
        "plotrange": (0, 15),
        "f": "f(x,a,b) = x**(a-1.0)*exp(-x/b)/(b**a*gamma(a))"
    },
    {
        "type": "lognormal",
        "params": [ ("zeta", 0.5), ("sigma", 0.25) ],
        "plotrange": (0, 3),
        "f": "f(x,z,s) = 1.0/(x*s*sqrt(2.0*pi))*exp(-(log(x)-z)**2/(2.0*s**2))"
    }
]

for s in settings:

    # Write config file for these settings
        
    lines = [ ]
    lines.append("test.dist.type = " + s["type"])
    for p in s["params"]:
        lines.append("test.dist." + s["type"] + "." + p[0] + " = " + str(p[1]))

    data = "\n".join(lines)

    fileName = "config-tmp-" + s["type"]
    with open(fileName, "wt") as f:
        f.write(data)
        f.close()

    # Run the executable with this config file

    outName = "out-tmp-" + s["type"]
    with open(outName, "wt") as f:
        subprocess.call( [ "../../../build_ninja2/testconfig-opt-debug", fileName ], stdout=f)
        f.close()

    # Process generated value in histogram

    h = histogram.Histogram(s["plotrange"][0], s["plotrange"][1], 100)
    with open(outName, "rt") as f:
        l = f.readline()
        while l:
            val = float(l)
            h.process(val)

            l = f.readline()

        f.close()

    histName = "hist-tmp-" + s["type"]
    with open(histName, "wt") as f:
        h.printProb(f)
        f.close()

    # Write gnuplot file
    plotName = "plot-tmp-" + s["type"] + ".gnuplot"
    pngName = "plot-tmp-" + s["type"] + ".png"
    with open(plotName, "wt") as f:
        print >>f, "set terminal png"
        print >>f, "set style data lines"
        print >>f, "set output '%s'" % pngName
        print >>f, s["f"]

        function = "f(x"
        for p in s["params"]:
            function += "," + str(float(p[1]))
        function += ") "

        print >>f, "plot [%g:%g] '%s', %s lt 3" % (s["plotrange"][0],s["plotrange"][1],histName,function)

    subprocess.call( [ "gnuplot", plotName ])
    os.unlink(plotName)
    os.unlink(histName)
    os.unlink(outName)
    os.unlink(fileName)
