% connect to VNA server
t = tcpclient('192.168.1.100', 1001);

% set start frequency in Hz
start = 100;
write(t, int32(0 * 2^28 + start));

% set stop frequency in Hz
stop = 100000;
write(t, int32(1 * 2^28 + stop));

% set sweep size
size = 1000;
write(t, int32(2 * 2^28 + size));

% set decimation rate
rate = 1000;
write(t, int32(3 * 2^28 + rate));

% set frequency correction
corr = 0;
write(t, int32(4 * 2^28 + corr));

% set output levels
level = 32766;
write(t, int32(5 * 2^28 + level));
level = 0;
write(t, int32(6 * 2^28 + level));

% set gpio pins
gpio = 1;
write(t, int32(7 * 2^28 + gpio));

% sweep
write(t, int32(8 * 2^28));

% read data
data = read(t, 4 * size, 'single');

% convert data
data = reshape(data, 4, size);
re1 = data(1, 1:size);
im1 = data(2, 1:size);
re2 = data(3, 1:size);
im2 = data(4, 1:size);
in1 = complex(re1, im1);
in2 = complex(re2, im2);

% disconnect from VNA server
delete(t);
clear t;
