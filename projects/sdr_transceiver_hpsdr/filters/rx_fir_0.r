library(signal)

Fo <- 0.0072                   # Pass band edge; 0.22*4/5/25

# fir2 parameters
k <- kaiserord(c(Fo, Fo+0.0084), c(1, 0), 1/(2^16), 1)
L <- k$n                       # Filter order
Beta <- k$beta                 # Kaiser window parameter

# FIR filter design using fir2
s <- 0.0001                    # Step size
fp <- seq(0.0, Fo, by=s)       # Pass band frequency samples
fs <- seq(Fo+0.0084, 0.5, by=s) # Stop band frequency samples
f <- c(fp, fs)*2               # Normalized frequency samples; 0<=f<=1

Mp <- matrix(1, 1, length(fp)) # Pass band response; Mp[1]=1
Mf <- c(Mp, matrix(0, 1, length(fs)))

h <- fir2(L, f, Mf, window=kaiser(L+1, Beta))

# Print filter coefficients
paste(as.character(h), collapse=", ")

fh <- freqz(h)

op <- par(mfrow = c(1, 2))

plot(f, Mf, type = "b", ylab = "magnitude", xlab = "Frequency", xlim = c(0, 0.04))
lines(fh$f / pi, abs(fh$h), col = "blue")

plot(f, 20*log10(Mf+1e-5), type = "b", ylab = "dB", xlab = "Frequency", xlim = c(0, 0.04))
lines(fh$f / pi, 20*log10(abs(fh$h)), col = "blue")

par(op)
