#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include "Wave.h"

using namespace std;

#define DO3  261.63
#define RE3  293.66
#define MI3  329.63
#define FA3  349.23
#define SOL3 392.00
#define LA3  440.00
#define SI3  493.88
#define PI  3.14159265358979323846
#define NB_NOTES 7

double NOTES[NB_NOTES] = {DO3, RE3, MI3, FA3, SOL3, LA3, SI3};

void add_note(vector<double>& signal, double freq_note, double duration, double freq_ech, double amplitude = 1.0);

// signal in [-1,-1] -> [0, 255]
unsigned char normalize(double signal_ech);

vector<unsigned char> to_data8(vector<double>& signal);

void write_signal(string& filepath, vector<double>& signal, int nb_channels, int sampling_freq);

vector<double> read_signal(string filepath, int nb_channels, int sampling_freq);

/*
	This FFT has been proposed by Paul Bourke
	http://paulbourke.net/miscellaneous/dft/
	This computes an in-place complex-to-complex FFT
	x and y are the real and imaginary arrays of 2^m points.
	dir =  1 gives forward transform
	dir = -1 gives reverse transform
	You MUST compute first the value m such that
	2^(m-1) < n (size of your signal) <= 2^m
	allocate a new signal of nm=2^m values
	then fill the n first values of this new signal
	with your signal and fill the rest with 0
	WARNING : you must pass m, not nm !!!
*/
int FFT(int dir, int m, double* x, double* y);

vector<double> FFT_visualize(vector<double>& signal_real, vector<double>& signal_imaginary);

void normalisation(double* signal, int N);


/* Renvoie m tel que 2^m >= n  */
int next_pow2(int n) {

    int next_pow2 = 1;
    int m = 0;

    while (next_pow2 < n) {
        next_pow2 *= 2;
        m++;
    }

    return m;
}

void add_gamme(vector<double>& signal, vector<double>& gamme, int sampling_freq){
    double max_freq = sampling_freq / 2.0;
    double max_index = signal.size() / 2.0;

    for (size_t i = 0; i < gamme.size(); ++i){
        size_t index = (max_index * gamme[i]) / max_freq;
        size_t index_sym = max_index + (max_index - index);
        signal[index] = 1;
        signal[index_sym] = 1;
    }
}


void filtre_passe_haut(double *signalReal,double *signalImaginaire, int N, double frequence, int i);
void filtre_passe_bas(double *signalReal,double *signalImaginaire, int N, double frequence, int i);
void filtre_passe_bande(double *signalReal,double *signalImaginaire, int N, double fc1,double fc2, int i);
void filtre_coupe_bande(double *signalReal,double *signalImaginaire, int N, double fc1,double fc2, int i);
void filtre_Butterworth(double *Input, double *Output, int N, double fc);
void filtre_passe_haut_temporelle(double *Input, double *Output, int N, double fc,double K);
int main(int argc, const char *argv[]) {

    cout << "Initialisation du signal...\n";

    double sample_time = 2; // en secondes
    string filename = "sounds/La.wav";
    string filename_FFT = "sounds/LaFFT.wav";
    string filename_IFFT = "sounds/LaIFFT.wav";
    string filename_Butterworth = "sounds/GammeButterworth.wav";
    string filename_passe_haut_temporelle= "sounds/gammePasseHautTemporelle.wav";
    int sampling_freq = 44100;
    int nb_channels = 1;

    vector<double> signal = read_signal(filename, nb_channels, sampling_freq);

    //add_note(signal, 0, 5, sampling_freq);

    /* les signaux doivent avoir une taille en puissance de deux pour que la FFT
       puisse fonctionnée convenablement */

    int FFT_m = next_pow2(signal.size());
    long long FFT_size = pow(2, FFT_m);

    cout << FFT_m << " " << FFT_size << "\n";

    vector<double> signal_real(FFT_size);
    vector<double> signal_imaginary(FFT_size);
    copy(signal.begin(), signal.end(), signal_real.begin());

    vector<double> signalButterworth(signal.size());
    vector<double> signalPasseHautTemporelle(signal.size());
    cout << "Transformation de fourrier en cours..." << endl;

    FFT(1, FFT_m, signal_real.data(), signal_imaginary.data());
    double max = 0;
    vector<double> norme(signal_real.size());
    for (size_t i = 0; i < signal_real.size(); ++i) {
        norme[i] = sqrt(signal_real[i] * signal_real[i] + signal_imaginary[i] * signal_imaginary[i]);
        max = max < norme[i] ? norme[i] : max;
    }

//    for (size_t i = 0; i < signal_real.size(); ++i) {
//        signal_real[i]= (2.0 * norme[i] / max) - 1.0;
//    }


//    filtre_passe_haut_temporelle(signal.data(),signalPasseHautTemporelle.data(),signal.size(),RE3,22.11);
//    write_signal(filename_passe_haut_temporelle, signalPasseHautTemporelle, nb_channels, sampling_freq);
//    filtre_Butterworth(signal.data(),signalButterworth.data(),signal.size(),RE3);
//    write_signal(filename_Butterworth, signalButterworth, nb_channels, sampling_freq);

//    filtre_passe_haut(signal_real.data(),signal_imaginary.data(), signal_real.size(), RE3, sampling_freq);
//   filtre_passe_bas(signal_real.data(),signal_imaginary.data(), signal_real.size(), SI3*100000000, sampling_freq);
//   filtre_passe_bande(signal_real.data(),signal_imaginary.data(), signal_real.size(), RE3,FA3, sampling_freq);
   filtre_coupe_bande(signal_real.data(),signal_imaginary.data(), signal_real.size(), RE3,FA3, sampling_freq);

        write_signal(filename_FFT, signal_real, nb_channels, sampling_freq);
//    write_signal(filename_FFT, signal_real, nb_channels, sampling_freq);

    FFT(-1, FFT_m, signal_real.data(), signal_imaginary.data());
    cout << "Ecriture du signal dans " << filename << "\n";
    copy(signal_real.begin(), signal_real.begin() + signal.size(), signal.begin());
    normalisation(signal.data(), signal.size());
    write_signal(filename_IFFT, signal, nb_channels, sampling_freq);

    return 0;
}

void filtre_passe_haut(double *signalReel,double *signalImaginaire, int N, double frequence, int sampling_freq) {
    frequence = frequence*(N/sampling_freq);
    for (int i = 0; i < N; ++i) {
        if (i < frequence or i > N-frequence  ){
            signalReel[i]= 0;
            signalImaginaire[i] = 0 ;
        }

    }
}

void filtre_passe_bas(double *signalReel,double *signalImaginaire, int N, double frequence, int sampling_freq) {
    frequence = frequence*(N/sampling_freq);
    for (int i = 0; i < N; ++i) {
        if (i > frequence and i < N-frequence  ){
            signalReel[i]= 0;
            signalImaginaire[i] = 0 ;
        }

    }
}

void filtre_passe_bande(double *signalReal,double *signalImaginaire, int N, double fc1,double fc2, int i){
    filtre_passe_haut(signalReal,signalImaginaire,N,fc1,i);
    filtre_passe_bas(signalReal,signalImaginaire,N,fc2,i);

}
void filtre_coupe_bande(double *signalReal,double *signalImaginaire, int N, double fc1,double fc2, int i){
    fc1 = fc1*(N/44100);
    fc2 = fc2*(N/44100);
    for (int i = 0; i < N; ++i) {
        if (i > fc1 and i<fc2 or i < N-fc1 and i>N-fc2  ){
            signalReal[i]= 0;
            signalImaginaire[i] = 0 ;
        }

    }

}

void filtre_Butterworth(double *Input, double *Output, int N, double fc){
    double alpha = M_PI*(fc / 41100);
    double A= 1 + alpha*(2  + 2*alpha + alpha*alpha);
    double B= -3 + alpha*(-2  + 2*alpha + 3*alpha*alpha);
    double C= 3 + alpha*(-2  -2*alpha + 3*alpha*alpha);
    double D= -1 + alpha*(2 -2*alpha + alpha*alpha);

    for(int i = 0; i < N; i++){
        if(i<=2){
            Output[i] = Input[i];
        }
        else {
            Output[i] = 1/((alpha * alpha * alpha * (Input[i - 3] + 3 * Input[i - 2] + 3 * Input[i - 1] + Input[i]) -
                         Output[i - 1] * B - Output[i - 2] * C - Output[i - 3] * D)) / A;
        }
    }

}

void filtre_passe_haut_temporelle(double *Input, double *Output, int N, double fc,double K){
    double alpha =  M_PI*(fc / 44100);
    double alpha_2 = alpha*alpha;
    for(int i = 0; i < N; i++){
        if(i<=2){
            Output[i] = Input[i];
        }
        else {
            Output[i] = Output[i-3]+Output[i-2]-Output[i-1] + (K*alpha_2*(-Input[i-3]-2*Input[i-2]+Input[i-1]+2*Input[i]))/alpha_2 ;
//            cout<<Output[i]<<"\n";
        }
    }
}
/*
S(t) = sin(2 * PI * freq * t)
S(n) = S(n * tau) = sin(2 * Pi * freq * n * tau) = sin(2 * PI * freq * 1 / (freq * tau) * n
*/

void add_note(vector<double>& signal, double freq_note, double duration, double freq_ech, double amplitude){

    long long nb_ech = (long long)(duration * freq_ech);

    for (long long i = 0; i < nb_ech; ++i)
        signal.push_back(amplitude * sin(2 * M_PI * freq_note * (i / freq_ech)));
}

// signal in [-1,-1] -> [0, 255]
unsigned char normalize(double signal_ech){
    return (signal_ech + 1.0) * 127.5;
}

vector<unsigned char> to_data8(vector<double>& signal) {

    vector<unsigned char> data8(signal.size());

    for (size_t i = 0; i < signal.size(); ++i)
        data8[i] = normalize(signal[i]);

    return move(data8);
}


void write_signal(string& filename, vector<double>& signal, int nb_channels, int sampling_freq) {

    vector<unsigned char> data8(to_data8(signal));

    Wave wave(data8.data(), data8.size(), nb_channels, sampling_freq);

    // convertie en char* car la fonction write de modifie pas la constante de toute façon
    wave.write((char*)filename.c_str());
}

vector<double> read_signal(string filepath, int nb_channels, int sampling_freq) {

    unsigned char* data8 = NULL;
    int size = 0;
    Wave wave(data8, size, nb_channels, sampling_freq);
    wave.read((char*)filepath.c_str());

    wave.getData8(&data8, &size);

    vector<double> signal(size);

    for (int i = 0; i < size; ++i)
        signal[i] = ((data8[i] * 2.0) / 255.0) - 1;

    delete[] data8;

    return move(signal);
}

void normalisation(double* signal, int N) {

    double max, min;
    double *pt, *fin;

    pt = signal;
    fin = pt + N;
    min = (*pt);
    max = (*pt);

    while (pt < fin) {
        min = (*pt) < min ? (*pt) : min;
        max = (*pt) > max ? (*pt) : max;
        pt++;
    }

    if(max > min + 1e-8)
        max = 2.0 / (max - min);
    else
        max = 1e-8;

    pt = signal;

    while (pt < fin) {
        (*pt) = (((*pt) - min) * max) - 1.0;
        pt++;
    }
}

vector<double> FFT_visualize(vector<double>& signal_real, vector<double>& signal_imaginary) {

    double max = 0;
    vector<double> norme(signal_real.size());

    for (size_t i = 0; i < signal_real.size(); ++i) {
        norme[i] = sqrt(signal_real[i] * signal_real[i] + signal_imaginary[i] * signal_imaginary[i]);
        max = max < norme[i] ? norme[i] : max;
    }

    /* normalize val -> [-1, 1] */

    for (size_t i = 0; i < signal_real.size(); ++i) {
        norme[i] = (2.0 * norme[i] / max) - 1.0;
    }

    return move(norme);
}

int FFT(int dir, int m, double *x, double *y) {

    int n,i,i1,j,k,i2,l,l1,l2;
    double c1,c2,tx,ty,t1,t2,u1,u2,z;

    /* Calculate the number of points */

    n = 1;

    for (i = 0 ; i < m ; i++)
        n *= 2;

    /* Do the bit reversal */

    j = 0;
    i2 = n >> 1;

    for (i = 0 ; i < n - 1 ; i++) {

        if (i < j) {
            tx = x[i];
            ty = y[i];
            x[i] = x[j];
            y[i] = y[j];
            x[j] = tx;
            y[j] = ty;
        }

        k = i2;

        while (k <= j) {
            j -= k;
            k >>= 1;
        }

        j += k;
    }

    /* Compute the FFT */

    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;

    for (l = 0 ; l < m ; l++) {

        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;

        for (j = 0 ; j < l1 ; j++) {

            for (i = j ; i < n ; i += l2) {
                i1 = i + l1;
                t1 = u1 * x[i1] - u2 * y[i1];
                t2 = u1 * y[i1] + u2 * x[i1];
                x[i1] = x[i] - t1;
                y[i1] = y[i] - t2;
                x[i] += t1;
                y[i] += t2;
            }

            z =  u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }

        c2 = sqrt((1.0 - c1) / 2.0);

        if (dir == 1)
            c2 = -c2;

        c1 = sqrt((1.0 + c1) / 2.0);
    }

    /* Scaling for forward transform */

    if (dir == 1) {
        for (i = 0 ; i < n ; i++) {
            x[i] /= n;
            y[i] /= n;
        }
    }

    return 1;
}