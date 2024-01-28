library(signal)

a0 <- 0.35875
a1 <- 0.48829
a2 <- 0.14128
a3 <- 0.01168
x <- seq(0.0, 1.0, by = 1 / 32)
h <- a0 - a1 * cos(2 * pi * x) + a2 * cos(4 * pi * x) - a3 * cos(6 * pi * x)
h <- h / sum(h)

# Print filter coefficients
paste(sprintf("%.10e", h), collapse = ", ")

fh <- freqz(h)

op <- par(mfrow = c(1, 2))

plot(fh$f / pi, abs(fh$h), type = "l", ylab = "magnitude", xlab = "Frequency", xlim = c(0, 0.25))

plot(fh$f / pi, 20*log10(abs(fh$h)+1e-5), type = "l", ylab = "dB", xlab = "Frequency", xlim = c(0, 0.25))

par(op)
