clear all;
close all;
clc

% Make an input signal
N = 100000;
t = linspace(0,100,N);
% Two phase with added gaussian noise 100 sec signal
current = cos(2*pi*50*t)+cos(2*pi*50*t+(2*pi/3));
AWGN = 0.001*randn(1,N);
raw_input = current+AWGN;

fs = N/100;

% Period finder
autocorr(current,100);


% Collect a 1 second package
raw_package = raw_input(1:1000);

% Sample signal package
k=1;
n=1;
for i=1:length(raw_package)
    if(k==11)
       sampled_input(n)=raw_package(i);
       n=n+1; 
       k=1;
    end
    k=k+1;
end


% Pick max value
max_val=0;

for i=1:length(sampled_input)
    if max_val<sampled_input(i)
        max_val=sampled_input(i);
    end
end

figure
plot(t,raw_input);

figure
plot(raw_package)

figure
plot(sampled_input);

mean_value=mean(abs(sampled_input));
