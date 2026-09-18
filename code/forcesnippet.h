class ForceGravitation : public Force
{
public:
    ForceGravitation() { attractor=nullptr; }
    ForceGravitation(const Particle* p, double k) { attractor = p, G = k; }
    virtual ~ForceGravitation() {}

    virtual void apply();

    void setAttractor(const Particle* p) { attractor = p; }
    const Particle* getAttractor() const { return attractor; }
    void setConstant(double k) { G = k; }
    void setSmoothingFactors(double sa, double sb) { a = sa; b = sb; }
    double getConstant() const { return G; }

protected:
    const Particle* attractor;
    double G = 6.6743e-11; // gravitational constant
    double a = 1, b = 1;
};