// =============================================================
// EA2. WattControl: Gestor de energía para Minimercado (El Niño 2026)
// Estudiante: Saira Dayana Ricardo - Entrega Individual
// =============================================================
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

#define HORAS_DIA 24
#define MAX_CARGAS 5

// Constantes del Negocio: Minimercado (Documento terminado en 8)
const float POTENCIA_CONTRATADA = 8.5;   // kW máximos contratados
const float PRECIO_CARO = 800.0;         // COP/kWh: límite para aplazar
const float EMBALSES_CRITICO = 40.0;     // %: límite para aplazar
const float PRECIO_ESCASEZ = 1500.0;     // COP/kWh: límite para emergencia (break)
const int PICO_INICIO = 18;              // 6:00 pm
const int PICO_FIN = 21;                 // 9:00 pm
const int FALLA_CADA = 20;               // Falla de sensor (continue)

const int MIN_HORAS_HORNO = 5;
const int MIN_HORAS_AIRE = 4;

// 1. Clase Base Abstracta
class Carga {
protected:
    string nombre;
    float potenciaKW;
    bool encendida;
    int horasUso;
    float kwhConsumidos;

public:
    Carga(string n, float p) {
        nombre = n;
        potenciaKW = p;
        encendida = false;
        horasUso = 0;
        kwhConsumidos = 0.0;
    }

    virtual ~Carga() {}

    virtual bool debeEncender(int hora, float precio, float embalses) = 0;

    void encender() {
        encendida = true;
        horasUso++;
        kwhConsumidos += potenciaKW;
    }

    void apagar() { encendida = false; }

    void reiniciar() {
        encendida = false;
        horasUso = 0;
        kwhConsumidos = 0.0;
    }

    string getNombre() { return nombre; }
    float getPotencia() { return potenciaKW; }
    bool estaEncendida() { return encendida; }
    int getHorasUso() { return horasUso; }
    float getKwhConsumidos() { return kwhConsumidos; }
};

// 2. Herencia: CargaCritica (Congelador)
class CargaCritica : public Carga {
public:
    CargaCritica(string n, float p) : Carga(n, p) {}

    bool debeEncender(int hora, float precio, float embalses) override {
        return true; // Nunca se apaga para conservar alimentos
    }
};

// 3. Herencia: CargaAplazable (Horno y Aire Acondicionado)
class CargaAplazable : public Carga {
private:
    int horasMinimasRequeridas;

public:
    CargaAplazable(string n, float p, int minHoras) : Carga(n, p) {
        horasMinimasRequeridas = minHoras;
    }

    int getHorasMinimas() { return horasMinimasRequeridas; }

    bool debeEncender(int hora, float precio, float embalses) override {
        bool horaPico = (hora >= PICO_INICIO && hora <= PICO_FIN);
        bool embalsesBajos = (embalses < EMBALSES_CRITICO);
        bool precioElevado = (precio > PRECIO_CARO);

        return !(embalsesBajos || horaPico || precioElevado);
    }
};

// 4. Medidor (Composición)
class Medidor {
private:
    float totalKWhConGestor;
    float costoTotalConGestor;
    float totalKWhSinGestor;
    float costoTotalSinGestor;

public:
    Medidor() { reiniciar(); }

    void reiniciar() {
        totalKWhConGestor = 0;
        costoTotalConGestor = 0;
        totalKWhSinGestor = 0;
        costoTotalSinGestor = 0;
    }

    void acumularConGestor(float kwh, float precio) {
        totalKWhConGestor += kwh;
        costoTotalConGestor += (kwh * precio);
    }

    void acumularSinGestor(float kwh, float precio) {
        totalKWhSinGestor += kwh;
        costoTotalSinGestor += (kwh * precio);
    }

    float getCostoConGestor() { return costoTotalConGestor; }
    float getCostoSinGestor() { return costoTotalSinGestor; }
};

// 5. Mercado (Asociación)
class Mercado {
public:
    float precioHora(int hora) {
        int minimo, maximo;
        if (hora <= 6) { minimo = 400; maximo = 650; }
        else if (hora <= 14) { minimo = 480; maximo = 950; }
        else if (hora <= 17) { minimo = 600; maximo = 1000; }
        else if (hora <= 21) { minimo = 750; maximo = 1450; }
        else { minimo = 500; maximo = 900; }
        
        return minimo + rand() % (maximo - minimo + 1);
    }

    float nivelEmbalses() {
        return 35.0 + (rand() % 46);
    }
};

// 6. Negocio: Minimercado (Agregación)
class Negocio {
private:
    string nombre;
    Medidor medidor;
    Carga* cargas[MAX_CARGAS];
    int numCargas;
    Mercado* mercado;

    int horasSimuladas;
    int fallas;
    bool racionamiento;

public:
    Negocio(string n, Mercado* m) {
        nombre = n;
        mercado = m;
        numCargas = 0;
        horasSimuladas = 0;
        fallas = 0;
        racionamiento = false;
    }

    void agregarCarga(Carga* c) {
        if (numCargas < MAX_CARGAS) {
            cargas[numCargas] = c;
            numCargas++;
        }
    }

    void aplicarRegla6() {
        float potenciaTotal = 0;
        for (int i = 0; i < numCargas; i++) {
            if (cargas[i]->estaEncendida()) {
                potenciaTotal += cargas[i]->getPotencia();
            }
        }

        if (potenciaTotal > POTENCIA_CONTRATADA) {
            cout << "  [ALERTA REGLA 6] Potencia solicitada (" << potenciaTotal 
                 << " kW) supera el límite (" << POTENCIA_CONTRATADA << " kW)." << endl;
            
            CargaAplazable* mayorAplazable = nullptr;
            for (int i = 0; i < numCargas; i++) {
                CargaAplazable* apl = dynamic_cast<CargaAplazable*>(cargas[i]);
                if (apl && apl->estaEncendida()) {
                    if (mayorAplazable == nullptr || apl->getPotencia() > mayorAplazable->getPotencia()) {
                        mayorAplazable = apl;
                    }
                }
            }

            if (mayorAplazable != nullptr) {
                mayorAplazable->apagar();
                cout << "  -> Regla 6 Aplicada: Se apaga " << mayorAplazable->getNombre() << endl;
            }
        }
    }

    void evaluar(int hora, float precio, float embalses) {
        cout << "\n--- Evaluación Manual | Hora " << hora << ":00 | Precio: $" << precio 
             << " | Embalses: " << embalses << "% ---" << endl;

        for (int i = 0; i < numCargas; i++) {
            if (cargas[i]->debeEncender(hora, precio, embalses)) cargas[i]->encender();
            else cargas[i]->apagar();
        }

        aplicarRegla6();

        for (int i = 0; i < numCargas; i++) {
            cout << "  " << setw(22) << left << cargas[i]->getNombre() << right
                 << (cargas[i]->estaEncendida() ? "ENCENDIDA" : "APAGADA / APLAZADA") << endl;
        }
    }

    void simularDia() {
        medidor.reiniciar();
        horasSimuladas = 0;
        fallas = 0;
        racionamiento = false;

        for (int i = 0; i < numCargas; i++) cargas[i]->reiniciar();

        float embalses = mercado->nivelEmbalses();
        float potenciaSinGestor = 0;
        for (int i = 0; i < numCargas; i++) potenciaSinGestor += cargas[i]->getPotencia();

        cout << "\n==========================================================================" << endl;
        cout << " SIMULACIÓN 24 HORAS - " << nombre << " (Embalses: " << embalses << "%)" << endl;
        cout << "==========================================================================" << endl;
        cout << fixed << setprecision(0);
        cout << setw(5) << "Hora" << setw(8) << "Precio";
        for (int i = 0; i < numCargas; i++) cout << setw(18) << cargas[i]->getNombre();
        cout << setw(10) << "Costo" << endl;

        for (int h = 0; h < HORAS_DIA; h++) {
            float precio = mercado->precioHora(h);

            medidor.acumularSinGestor(potenciaSinGestor, precio);

            if (precio > PRECIO_ESCASEZ && embalses < EMBALSES_CRITICO) {
                cout << setw(5) << h << setw(8) << precio 
                     << "  EMERGENCIA DE RACIONAMIENTO: Se detiene la simulación" << endl;
                racionamiento = true;
                break;
            }

            horasSimuladas++;

            if (rand() % FALLA_CADA == 0) {
                fallas++;
                float kwhHora = 0;
                for (int i = 0; i < numCargas; i++) {
                    CargaCritica* crit = dynamic_cast<CargaCritica*>(cargas[i]);
                    if (crit) {
                        crit->encender();
                        kwhHora += crit->getPotencia();
                    } else {
                        cargas[i]->apagar();
                    }
                }
                medidor.acumularConGestor(kwhHora, precio);
                cout << setw(5) << h << setw(8) << "--" 
                     << "  Falla de lectura: Solo carga crítica encendida" << endl;
                continue;
            }

            for (int i = 0; i < numCargas; i++) {
                if (cargas[i]->debeEncender(h, precio, embalses)) cargas[i]->encender();
                else cargas[i]->apagar();
            }

            aplicarRegla6();

            float kwhHora = 0;
            for (int i = 0; i < numCargas; i++) {
                if (cargas[i]->estaEncendida()) kwhHora += cargas[i]->getPotencia();
            }

            float costoH = kwhHora * precio;
            medidor.acumularConGestor(kwhHora, precio);

            cout << setw(5) << h << setw(8) << precio;
            for (int i = 0; i < numCargas; i++) {
                cout << setw(18) << (cargas[i]->estaEncendida() ? "ON" : "off");
            }
            cout << setw(10) << costoH << endl;
        }

        cout << "\nSimulación finalizada. Ver el resumen en la opción 4." << endl;
    }

    void resumen() {
        if (horasSimuladas == 0) {
            cout << "\nPrimero simule un día con la opción 3." << endl;
            return;
        }

        cout << "\n=======================================================" << endl;
        cout << "       RESUMEN Y ANÁLISIS DE AHORRO - " << nombre << endl;
        cout << "=======================================================" << endl;
        cout << "Horas simuladas: " << horasSimuladas << " de " << HORAS_DIA 
             << " | Fallas de lectura: " << fallas << endl;
        cout << "-------------------------------------------------------" << endl;

        bool produccionIncompleta = false;

        for (int i = 0; i < numCargas; i++) {
            cout << "  " << setw(22) << left << cargas[i]->getNombre() << right
                 << cargas[i]->getHorasUso() << " h, "
                 << fixed << setprecision(1) << cargas[i]->getKwhConsumidos() << " kWh" << endl;

            CargaAplazable* apl = dynamic_cast<CargaAplazable*>(cargas[i]);
            if (apl) {
                if (apl->getHorasUso() < apl->getHorasMinimas()) {
                    produccionIncompleta = true;
                }
            }
        }

        cout << "-------------------------------------------------------" << endl;
        float costoCon = medidor.getCostoConGestor();
        float costoSin = medidor.getCostoSinGestor();
        float ahorroCOP = costoSin - costoCon;
        float ahorroPct = (costoSin > 0) ? (ahorroCOP / costoSin) * 100.0 : 0.0;

        cout << fixed << setprecision(0);
        cout << "Costo sin gestor (3 cargas encendidas 24h) : $" << costoSin << " COP" << endl;
        cout << "Costo con gestor (WattControl)             : $" << costoCon << " COP" << endl;
        cout << "Ahorro obtenido                            : $" << ahorroCOP << " COP";
        if (costoSin > 0) cout << " (" << setprecision(1) << ahorroPct << " %)";
        cout << endl;

        if (produccionIncompleta) {
            cout << "\n[ALERTA] Producción incompleta (no se alcanzaron las horas mínimas)." << endl;
        } else {
            cout << "\n[ÉXITO] Producción completa." << endl;
        }

        if (racionamiento) {
            cout << "[NOTA] Simulación interrumpida por emergencia de racionamiento." << endl;
        }
        cout << "=======================================================" << endl;
    }
};

int main() {
    srand(time(0));

    Mercado mercado;

    // Cargas del Minimercado (Dígito 8)
    CargaCritica congelador("Congelador Lácteos", 2.5);
    CargaAplazable horno("Horno Panadería", 4.5, MIN_HORAS_HORNO);
    CargaAplazable aire("Aire Acondicionado", 3.0, MIN_HORAS_AIRE);

    Negocio negocio("Minimercado El Súper 8", &mercado);
    negocio.agregarCarga(&congelador);
    negocio.agregarCarga(&horno);
    negocio.agregarCarga(&aire);

    int hora = 12;
    float embalses = 60.0, precio = 500.0;
    bool haySimulacion = false;
    int opcion;

    do {
        cout << endl << "===== WattControl v1 - Minimercado =====" << endl;
        cout << "1. Ingresar lectura manual (hora, precio, embalses)" << endl;
        cout << "2. Evaluar decisión de cada carga" << endl;
        cout << "3. Simular un día completo (24 h)" << endl;
        cout << "4. Ver resumen de la última simulación" << endl;
        cout << "0. Salir" << endl;
        cout << "Opción: ";
        cin >> opcion;

        switch (opcion) {
        case 1:
            do {
                cout << "Hora (0 a 23): ";
                cin >> hora;
                if (hora < 0 || hora > 23) cout << "  Hora inválida." << endl;
            } while (hora < 0 || hora > 23);

            do {
                cout << "Precio de bolsa (COP/kWh): ";
                cin >> precio;
                if (precio <= 0) cout << "  El precio debe ser mayor que 0." << endl;
            } while (precio <= 0);

            do {
                cout << "Nivel de embalses (0 a 100 %): ";
                cin >> embalses;
                if (embalses < 0 || embalses > 100) cout << "  Nivel inválido." << endl;
            } while (embalses < 0 || embalses > 100);

            cout << "Lectura guardada." << endl;
            break;

        case 2:
            negocio.evaluar(hora, precio, embalses);
            break;

        case 3:
            negocio.simularDia();
            haySimulacion = true;
            break;

        case 4:
            if (haySimulacion) negocio.resumen();
            else cout << "Primero simule un día con la opción 3." << endl;
            break;

        case 0:
            cout << "Hasta luego." << endl;
            break;

        default:
            cout << "Opción inválida." << endl;
        }

        if (opcion != 0) {
            cout << endl << "Presione Enter para continuar...";
            cin.ignore();
            cin.get();
        }
    } while (opcion != 0);

    return 0;
}