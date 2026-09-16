#include <cmath>
#include <numbers>

inline double rads(double deg)
{
    return deg * (std::numbers::pi / 180);
}

// Equirectangular approximation
inline double calc_dist(double base_lat, double base_lon,
                        double targ_lat, double targ_lon)
{
    double phi1 = rads(base_lat);
    double phi2 = rads(targ_lat);

    double lambda1 = rads(base_lon);
    double lambda2 = rads(targ_lon);

    double sclr = std::cos((phi1 + phi2) / 2.0);

    double y = phi2 - phi1;
    double x = (lambda2 - lambda1) * sclr;

    return std::sqrt(x * x + y * y) * 3959.0;
}