#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cctype>
const double M_PI = 3.14159265358979323846;
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}
std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s;
}
double parseValue(const std::string& valStrInput) {
    std::string valStr = trim(valStrInput);
    if (valStr.empty()) throw std::invalid_argument("Empty value string");

    std::string numStr = valStr;
    char suffix = '\0';
    double multiplier = 1.0;

    if (!numStr.empty() && std::isalpha(static_cast<unsigned char>(numStr.back()))) {
        suffix = std::tolower(static_cast<unsigned char>(numStr.back()));
        numStr.pop_back();

        if ((suffix == 'g' || suffix == 'x') && numStr.length() >= 2) {
            std::string lastTwo = toLower(numStr.substr(numStr.length() - 2));
            if (lastTwo == "me") {
                numStr.resize(numStr.length() - 2);
                multiplier = 1e6;
            } else {
                multiplier = 1e6;
            }
        } else {
            switch (suffix) {
                case 'f': multiplier = 1e-15; break;
                case 'p': multiplier = 1e-12; break;
                case 'n': multiplier = 1e-9;  break;
                case 'u': multiplier = 1e-6;  break;
                case 'm': multiplier = 1e-3;  break;
                case 'k': multiplier = 1e3;   break;
                default:
                    numStr += suffix;
                    multiplier = 1.0;
                    break;
            }
        }
    }

    if (numStr.empty() && multiplier != 1.0) throw std::invalid_argument("Number part is empty with suffix.");
    if (numStr.empty() && valStr.length() > 0) throw std::invalid_argument("Invalid number format: " + valStrInput);
    try {
        return std::stod(numStr) * multiplier;
    } catch (const std::invalid_argument& e) {
        throw std::invalid_argument("Invalid number format: " + valStrInput);
    } catch (const std::out_of_range& e) {
        throw std::out_of_range("Number out of range: " + valStrInput);
    }
}
class Node {
public:
    std::string name;
    bool isGround;

    Node(std::string n) : name(std::move(n)), isGround(false) {}

    void setGround(bool ground) {
        isGround = ground;
    }
};
class CircuitElement {
public:
    std::string name;
    Node* node1;
    Node* node2;

    CircuitElement(std::string n, Node* n1, Node* n2)
            : name(std::move(n)), node1(n1), node2(n2) {}

    virtual ~CircuitElement() = default;
    virtual std::string getTypeString() const = 0;
    virtual std::string getDescription() const = 0;
    virtual double getValue() const { return 0.0; }

    virtual double getCurrent(double v1_now, double v2_now,
                              double v1_prev, double v2_prev,
                              double time_step_h,
                              double t_current) const {
        (void)v1_now; (void)v2_now; (void)v1_prev; (void)v2_prev;
        (void)time_step_h; (void)t_current;
        return 0.0;
    }
};
class Resistor : public CircuitElement {
public:
    double resistance;
    Resistor(const std::string& n, Node* n1, Node* n2, double r)
            : CircuitElement(n, n1, n2), resistance(r) {
        if (r <= 0) throw std::invalid_argument("Resistance cannot be zero or negative");
    }
    std::string getTypeString() const override { return "Resistor"; }
    std::string getDescription() const override {
        std::ostringstream oss;
        oss << "Resistor " << name << " (" << node1->name << " to " << node2->name << ") Value: " << resistance << " Ohms";
        return oss.str();
    }    double getValue() const override { return resistance; }

    double getCurrent(double v1, double v2, double v1_prev, double v2_prev, double time_step_h, double t_current) const override {
        (void)v1_prev; (void)v2_prev; (void)time_step_h; (void)t_current;
        return (v1 - v2) / resistance;
    }
};
class Capacitor : public CircuitElement {
public:
    double capacitance;
    Capacitor(const std::string& n, Node* n1, Node* n2, double c)
            : CircuitElement(n, n1, n2), capacitance(c) {
        if (c <= 0) throw std::invalid_argument("Capacitance cannot be zero or negative");
    }
    std::string getTypeString() const override { return "Capacitor"; }
    std::string getDescription() const override {
        std::ostringstream oss;
        oss << "Capacitor " << name << " (" << node1->name << " to " << node2->name << ") Value: " << capacitance << " Farads";
        return oss.str();
    }
    double getValue() const override { return capacitance; }

    double getCurrent(double v1_now, double v2_now, double v1_prev, double v2_prev, double time_step_h, double t_current) const override {
        (void)t_current;
        if (time_step_h == 0) return 0.0;
        double Vc_now = v1_now - v2_now;
        double Vc_prev = v1_prev - v2_prev;
        return capacitance * (Vc_now - Vc_prev) / time_step_h;
    }
};
class Inductor : public CircuitElement {
public:
    double inductance;
    mutable double current_MNA_solution = 0.0;

    Inductor(const std::string& n, Node* n1, Node* n2, double l)
            : CircuitElement(n, n1, n2), inductance(l) {
        if (l <= 0) throw std::invalid_argument("Inductance cannot be zero or negative");
    }
    std::string getTypeString() const override { return "Inductor"; }
    std::string getDescription() const override {
        std::ostringstream oss;
        oss << "Inductor " << name << " (" << node1->name << " to " << node2->name << ") Value: " << inductance << " Henrys";
        return oss.str();
    }
    double getValue() const override { return inductance; }


    double getCurrent(double v1, double v2, double v1_prev, double v2_prev, double time_step_h, double t_current) const override {
        (void)v1; (void)v2; (void)v1_prev; (void)v2_prev; (void)time_step_h; (void)t_current;
        return current_MNA_solution;
    }
};
class VoltageSource : public CircuitElement {
public:
    bool isSinusoidal = false;
    double dc_voltage = 0.0;
    double amplitude = 0.0;
    double frequency = 0.0;
    double phase_degrees = 0.0;

    mutable double current_MNA_solution = 0.0;

    VoltageSource(const std::string& n, Node* n1, Node* n2, double v_dc)
            : CircuitElement(n, n1, n2), dc_voltage(v_dc) {}

    VoltageSource(const std::string& n, Node* n1, Node* n2,
                  double offset_val, double amp_val, double freq_val, double phase_val = 0.0)
            : CircuitElement(n, n1, n2), dc_voltage(offset_val), amplitude(amp_val), frequency(freq_val), phase_degrees(phase_val) {
        isSinusoidal = true;
    }

    double getVoltageAtTime(double t) const {
        if (isSinusoidal) {
            double phase_rad = phase_degrees * M_PI / 180.0;
            return dc_voltage + amplitude * sin(2 * M_PI * frequency * t + phase_rad);
        }
        return dc_voltage;
    }

    std::string getTypeString() const override { return "VoltageSource"; }
    std::string getDescription() const override {
        std::ostringstream oss;
        oss << "VoltageSource " << name << " (" << node1->name << "(+) to " << node2->name << "(-)) Value: ";
        if(isSinusoidal) {
            oss << "SIN(" << dc_voltage << " " << amplitude << " " << frequency << "Hz " << phase_degrees << "deg)";
        } else {
            oss << dc_voltage;
        }
        oss << " Volts";
        return oss.str();
    }
    double getValue() const override { return getVoltageAtTime(0); }

    double getCurrent(double v1, double v2, double v1_prev, double v2_prev, double time_step_h, double t_current) const override {
        (void)v1; (void)v2; (void)v1_prev; (void)v2_prev; (void)time_step_h; (void)t_current;
        return current_MNA_solution;
    }
};
class CurrentSource : public CircuitElement {
public:
    bool isSinusoidal = false;
    double dc_current = 0.0;
    double amplitude = 0.0;
    double frequency = 0.0;
    double phase_degrees = 0.0;

    CurrentSource(const std::string& n, Node* n1, Node* n2, double i_dc)
            : CircuitElement(n, n1, n2), dc_current(i_dc) {}

    CurrentSource(const std::string& n, Node* n1, Node* n2,
                  double offset_val, double amp_val, double freq_val, double phase_val = 0.0)
            : CircuitElement(n, n1, n2), dc_current(offset_val), amplitude(amp_val), frequency(freq_val), phase_degrees(phase_val) {
        isSinusoidal = true;
    }

    double getCurrentAtTime(double t) const {
        if (isSinusoidal) {
            double phase_rad = phase_degrees * M_PI / 180.0;
            return dc_current + amplitude * sin(2 * M_PI * frequency * t + phase_rad);
        }
        return dc_current;
    }
    std::string getTypeString() const override { return "CurrentSource"; }
    std::string getDescription() const override {
        std::ostringstream oss;
        oss << "CurrentSource " << name << " (from " << node1->name << " to " << node2->name << ") Value: ";
        if(isSinusoidal) {
            oss << "SIN(" << dc_current << " " << amplitude << " " << frequency << "Hz " << phase_degrees << "deg)";
        } else {
            oss << dc_current;
        }
        oss << " Amps";
        return oss.str();
    }
    double getValue() const override { return getCurrentAtTime(0); }

    double getCurrent(double v1, double v2, double v1_prev, double v2_prev, double time_step_h, double t_current) const {
        (void)v1; (void)v2; (void)v1_prev; (void)v2_prev; (void)time_step_h;
        return getCurrentAtTime(t_current);
    }
};
class Diode : public CircuitElement {
public:
    enum class DiodeModelType { IDEAL, ZENER };
    DiodeModelType model_type;
    std::string type_char_raw;
    static constexpr double DEFAULT_G_ON = 1.0;
    static constexpr double DEFAULT_G_OFF = 1e-9;

    double G_on;
    double G_off;

    double zener_voltage_Vz;
    double forward_voltage_Vf;

    enum class OperatingState { OFF, FORWARD_BIASED, ZENER_BREAKDOWN };
    mutable OperatingState current_op_state;

    Diode(const std::string& n, Node* n1, Node* n2, const std::string& type_str, double p_zener_voltage = 0.0)
            : CircuitElement(n, n1, n2), type_char_raw(toLower(type_str)),
              G_on(DEFAULT_G_ON), G_off(DEFAULT_G_OFF), current_op_state(OperatingState::OFF) {

        if (type_char_raw == "d") {
            model_type = DiodeModelType::IDEAL;
            forward_voltage_Vf = 0.0;
            zener_voltage_Vz = 0.0;
        } else if (type_char_raw == "z") {
            model_type = DiodeModelType::ZENER;
            forward_voltage_Vf = 0.7;
            zener_voltage_Vz = p_zener_voltage;
            if (p_zener_voltage <= 0) {
                throw std::invalid_argument("Zener voltage (Vz) for diode " + name + " must be positive.");
            }
        } else {
            throw std::invalid_argument("Unknown diode type '" + type_str + "' for diode " + name);
        }
    }

    std::string getTypeString() const override {
        return (model_type == DiodeModelType::IDEAL) ? "IdealDiode" : "ZenerDiode";
    }

    std::string getDescription() const override {
        std::ostringstream oss;
        oss << ((model_type == DiodeModelType::IDEAL) ? "IdealDiode " : "ZenerDiode ") << name
            <<" ("<< node1->name << " to " << node2->name << ")";
        if (model_type == DiodeModelType::ZENER) {
            oss << " Vf=" << forward_voltage_Vf << "V Vz=" << zener_voltage_Vz << "V";
        } else {
            oss << " Vf=" << forward_voltage_Vf << "V";
        }
        return oss.str();
    }

    double getValue() const override {
        return (model_type == DiodeModelType::ZENER) ? zener_voltage_Vz : forward_voltage_Vf;
    }

    void updateOperatingState(double v_anode_prev, double v_cathode_prev) const {
        double Vd_prev = v_anode_prev - v_cathode_prev;

        if (Vd_prev >= forward_voltage_Vf) {
            current_op_state = OperatingState::FORWARD_BIASED;
        } else {
            if (model_type == DiodeModelType::ZENER && Vd_prev < -zener_voltage_Vz) {

                current_op_state = OperatingState::ZENER_BREAKDOWN;
            } else {
                current_op_state = OperatingState::OFF;
            }
        }
    }


    double getCurrent(double v1_now_anode, double v2_now_cathode,
                      double v1_prev, double v2_prev,
                      double time_step_h, double t_current) const override {
        (void)v1_prev; (void)v2_prev; (void)time_step_h; (void)t_current;

        double Vd_now = v1_now_anode - v2_now_cathode;
        double G_eff_for_calc, V_series_eq_for_calc;
        bool use_off_model = false;


        switch (current_op_state) {
            case OperatingState::FORWARD_BIASED:
                G_eff_for_calc = G_on;
                V_series_eq_for_calc = forward_voltage_Vf;
                if (Vd_now < forward_voltage_Vf) {
                    use_off_model = true;
                }
                break;
            case OperatingState::ZENER_BREAKDOWN:
                G_eff_for_calc = G_on;
                V_series_eq_for_calc = -zener_voltage_Vz;
                if (Vd_now > -zener_voltage_Vz) {
                    use_off_model = true;
                }
                break;
            case OperatingState::OFF:
            default:
                use_off_model = true;
                break;
        }

        if (use_off_model) {
            return Vd_now * G_off;
        } else {

            return G_eff_for_calc * (Vd_now - V_series_eq_for_calc);
        }
    }
};
class Schematic {
public:
    std::string name;
    std::vector<Node*> nodes;
    std::vector<CircuitElement*> elements;
    std::vector<std::string> netlistCommands;
    Node* groundNode = nullptr;
    std::map<std::string, double> node_voltages_prev_step;
    Schematic(std::string n = "untitled") : name(std::move(n)) {}
    ~Schematic() {
        clear();
    }
    void clear() {
        for (CircuitElement* el : elements) delete el;
        elements.clear();
        for (Node* n : nodes) delete n;
        nodes.clear();
        netlistCommands.clear();
        node_voltages_prev_step.clear();
        groundNode = nullptr;
    }
    Node* findOrCreateNode(const std::string& nodeName) {
        if (nodeName.empty()) throw std::runtime_error("Node name cannot be empty.");
        for (Node* n : nodes) {
            if (n->name == nodeName) return n;
        }
        Node* newNode = new Node(nodeName);
        if (toLower(nodeName) == "gnd" || nodeName == "0") {
            if (groundNode == nullptr) {
                newNode->setGround(true);
                groundNode = newNode;
                std::cout << "Node " << nodeName << " automatically set as ground." << std::endl;
            }
        }
        nodes.push_back(newNode);
        node_voltages_prev_step[nodeName] = 0.0;
        return newNode;
    }
    Node* findNode(const std::string& nodeName) const {
        for (Node* n : nodes) {
            if (n->name == nodeName) return n;
        }
        return nullptr;
    }
    CircuitElement* findElement(const std::string& elementName) const {
        for (CircuitElement* el : elements) {
            if (el->name == elementName) return el;
        }
        return nullptr;
    }
    void addElement(CircuitElement* el, const std::string& command) {
        if (findElement(el->name)) {
            std::string errorMsg = "Error: Element <" + el->name + "> already exists in the circuit";
            delete el;
            throw std::runtime_error(errorMsg);
        }
        elements.push_back(el);
        netlistCommands.push_back(command);
    }
    void deleteElementCommand(const std::string& elementFullNameWithPrefix) {
        std::stringstream ss(elementFullNameWithPrefix);
        std::string typePrefixStr, elementName;
        ss >> typePrefixStr;
        if (ss >> elementName) {} else { elementName = typePrefixStr; typePrefixStr = ""; }

        auto it = std::remove_if(elements.begin(), elements.end(),
                                 [&](CircuitElement* el) {
                                     if (el->name == elementName) { delete el; return true; }
                                     return false;
                                 });
        if (it == elements.end()) {
            std::string typeStr = "element";
            if (!typePrefixStr.empty() && std::isalpha(static_cast<unsigned char>(typePrefixStr[0]))){
                char prefix = std::toupper(static_cast<unsigned char>(typePrefixStr[0]));
                if(prefix == 'R') typeStr = "resistor"; else if (prefix == 'C') typeStr = "capacitor";
                else if (prefix == 'L') typeStr = "inductor"; else if (prefix == 'V') typeStr = "voltage source";
                else if (prefix == 'I') typeStr = "current source";
            }
            throw std::runtime_error("Error: Cannot delete " + typeStr +"; component " + elementName + " not found");
        }
        elements.erase(it, elements.end());
        netlistCommands.erase(
                std::remove_if(netlistCommands.begin(), netlistCommands.end(),
                               [&](const std::string& cmd) {
                                   std::stringstream cmd_ss(cmd); std::string firstWord, cmd_elType, cmd_elName;
                                   cmd_ss >> firstWord;
                                   if (toLower(firstWord) == "add") { cmd_ss >> cmd_elType >> cmd_elName; return cmd_elName == elementName; }
                                   else { return firstWord == elementName; }
                               }), netlistCommands.end());
        std::cout << "Element " << elementName << " deleted." << std::endl;
    }
    void addGroundCommand(const std::string& nodeName, const std::string& command) {
        Node* n = findOrCreateNode(nodeName);
        if (groundNode != nullptr && groundNode != n && groundNode->name != n->name) {

        }
        n->setGround(true); groundNode = n;
        node_voltages_prev_step[nodeName] = 0.0;

        bool commandExists = false;
        for(const auto& cmd : netlistCommands){
            if(cmd.find("add GND " + nodeName) != std::string::npos || cmd.find("add gnd " + nodeName) != std::string::npos ) {
                commandExists = true; break;
            }
        }
        if(!commandExists) netlistCommands.push_back(command);
    }
    void listNodes() const {
        if (nodes.empty()) { std::cout << "No nodes in the circuit." << std::endl; return; }
        std::cout << "Available nodes:" << std::endl;
        bool first = true;
        for (const auto& node : nodes) {
            if (!first) std::cout << ", ";
            std::cout << node->name << (node->isGround ? " (GND)" : "");
            first = false;
        }
        std::cout << std::endl;
    }
    void listElements(const std::string& typeFilter = "") const {
        if (elements.empty()) { std::cout << "No elements in the circuit." << std::endl; return; }
        bool found = false;
        std::cout << "Circuit elements" << (typeFilter.empty() ? "" : " (Filter: " + typeFilter + ")") << ":" << std::endl;
        for (CircuitElement* el : elements) {
            std::string elTypeShort;
            if (dynamic_cast<Resistor*>(el)) elTypeShort = "R"; else if (dynamic_cast<Capacitor*>(el)) elTypeShort = "C";
            else if (dynamic_cast<Inductor*>(el)) elTypeShort = "L"; else if (dynamic_cast<VoltageSource*>(el)) elTypeShort = "V";
            else if (dynamic_cast<CurrentSource*>(el)) elTypeShort = "I";
            else if (dynamic_cast<Diode*>(el)) elTypeShort = "D";
            else elTypeShort = "UNKNOWN";
            if (typeFilter.empty() || toLower(typeFilter) == toLower(elTypeShort) || toLower(typeFilter) == toLower(el->getTypeString())) {
                std::cout << "- " << el->getDescription() << std::endl; found = true;
            }
        }
        if (!found && !typeFilter.empty()) std::cout << "No elements of type '" << typeFilter << "' found." << std::endl;
    }
    void renameNode(const std::string& oldName, const std::string& newName) {
        Node* oldNodePtr = findNode(oldName);
        if (!oldNodePtr) { std::cout << "ERROR: Node <" << oldName << "> does not exist in the circuit" << std::endl; return; }
        if (findNode(newName)) { std::cout << "ERROR: Node name <" << newName << "> already exists" << std::endl; return; }
        if (newName.empty() || newName.find_first_of(" \t") != std::string::npos) { std::cout << "ERROR: Invalid syntax - correct format: .rename node <old_name> <new_name>" << std::endl; return; }

        if(node_voltages_prev_step.count(oldName)){
            double val = node_voltages_prev_step[oldName];
            node_voltages_prev_step.erase(oldName);
            node_voltages_prev_step[newName] = val;
        }


        oldNodePtr->name = newName;
        for (std::string& cmd : netlistCommands) {
            std::stringstream line_ss(cmd); std::string token; std::vector<std::string> tokens;
            while(line_ss >> token) tokens.push_back(token);
            bool changed = false; if (tokens.empty()) continue;
            if (toLower(tokens[0]) == "add" && tokens.size() >= 3) {
                if (toLower(tokens[1]) == "gnd" && tokens.size() == 3) { if (tokens[2] == oldName) { tokens[2] = newName; changed = true; } }
                else if (tokens.size() >= 5) { if (tokens[3] == oldName) { tokens[3] = newName; changed = true; } if (tokens[4] == oldName) { tokens[4] = newName; changed = true; } }
            } else if (tokens.size() >= 3 && std::isalpha(static_cast<unsigned char>(tokens[0][0])) && toLower(tokens[0]) != ".rename" && toLower(tokens[0]) != ".nodes" && toLower(tokens[0]) != ".list" ) {
                if (tokens[1] == oldName) { tokens[1] = newName; changed = true; } if (tokens[2] == oldName) { tokens[2] = newName; changed = true; }
            }
            if(changed) { std::string newCmd; for(size_t i=0; i < tokens.size(); ++i) newCmd += tokens[i] + (i == tokens.size() -1 ? "" : " "); cmd = newCmd; }
        }
        std::cout << "SUCCESS: Node renamed from <" << oldName << "> to <" << newName << ">" << std::endl;
    }
    void renameElement(const std::string& oldName, const std::string& newName) {
        CircuitElement* elementToRename = findElement(oldName);

        if (!elementToRename) {
            std::cout << "ERROR: Element <" << oldName << "> does not exist in the circuit." << std::endl;
            return;
        }

        if (findElement(newName)) {
            std::cout << "ERROR: Element name <" << newName << "> already exists." << std::endl;
            return;
        }

        if (newName.empty() || newName.find_first_of(" \t") != std::string::npos) {
            std::cout << "ERROR: Invalid new element name. Cannot be empty or contain whitespace." << std::endl;
            return;
        }


        if (!oldName.empty() && !newName.empty() && std::toupper(oldName[0]) != std::toupper(newName[0])) {
            std::cout << "Warning: Renaming element <" << oldName << "> to <" << newName
                      << "> changes its implied type prefix. This is unusual." << std::endl;
        }


        elementToRename->name = newName;

        for (std::string& cmd : netlistCommands) {
            std::stringstream line_ss(cmd);
            std::vector<std::string> tokens;
            std::string token;
            while (line_ss >> token) {
                tokens.push_back(token);
            }

            if (tokens.empty()) continue;

            bool changed = false;
            if (toLower(tokens[0]) == "add" && tokens.size() >= 3) {
                if (toLower(tokens[1]) != "gnd" && tokens.size() >= 3 && tokens[2] == oldName) {
                    tokens[2] = newName;
                    changed = true;
                }
            }
            else if (!tokens[0].empty() && tokens[0][0] != '.' && tokens[0] == oldName) {
                if (std::isalpha(static_cast<unsigned char>(tokens[0][0])) && std::isupper(static_cast<unsigned char>(tokens[0][0])) ) {
                    tokens[0] = newName;
                    changed = true;
                }
            }

            if (changed) {
                std::string newCmd;
                for (size_t i = 0; i < tokens.size(); ++i) {
                    newCmd += tokens[i] + (i == tokens.size() - 1 ? "" : " ");
                }
                cmd = newCmd;
            }
        }

        std::cout << "SUCCESS: Element renamed from <" << oldName << "> to <" << newName << ">." << std::endl;
    }
    void printNetlist() const {
        std::cout << name << ":" << std::endl;
        for (const auto& cmd : netlistCommands) {
            std::string tempCmd = cmd; std::stringstream ss(tempCmd); std::string firstWord, secondWord, thirdWord;
            ss >> firstWord;
            if(toLower(firstWord) == "add") {
                ss >> secondWord;
                if(toLower(secondWord) == "gnd"){ tempCmd.erase(0, 4); }
                else { ss >> thirdWord; std::string restOfLine; std::getline(ss, restOfLine); tempCmd = thirdWord + restOfLine; }
            } else if (tempCmd.rfind(".rename node ",0) == 0 || tempCmd.rfind(".nodes",0)==0 || tempCmd.rfind(".list",0)==0){ continue; }
            std::cout << tempCmd << std::endl;
        }
    }
    void simulateTransient(double t_end, double time_step_h) {

        std::map<std::string, double> fixedNodeVoltageAtT;
        std::vector<VoltageSource*> floatingVSources;
        std::vector<Inductor*> inductors_in_circuit;
        std::vector<Diode*> diodes_in_circuit;
        std::map<std::string, double> initialFixedNodeVoltages_dc;
        for (CircuitElement* el : elements) {
            if (VoltageSource* v = dynamic_cast<VoltageSource*>(el)) {
                double dc_val = v->getVoltageAtTime(0.0);
                if (v->node2->isGround) { initialFixedNodeVoltages_dc[v->node1->name] = dc_val; }
                else if (v->node1->isGround) { initialFixedNodeVoltages_dc[v->node2->name] = -dc_val; }
                else { floatingVSources.push_back(v); }
            }
            else if (Inductor* l = dynamic_cast<Inductor*>(el)) {
                inductors_in_circuit.push_back(l);
            }
            else if (Diode* d = dynamic_cast<Diode*>(el)) {
                diodes_in_circuit.push_back(d);
            }
        }

        std::map<std::string, int> unknownNodeMap;
        int current_matrix_index = 0;
        for (Node* node : nodes) {
            if (!node->isGround && initialFixedNodeVoltages_dc.find(node->name) == initialFixedNodeVoltages_dc.end()) {
                unknownNodeMap[node->name] = current_matrix_index++;
            }
            if(node_voltages_prev_step.find(node->name) == node_voltages_prev_step.end()){
                node_voltages_prev_step[node->name] = 0.0;
            }
        }
        if(groundNode && node_voltages_prev_step.find(groundNode->name) == node_voltages_prev_step.end()){
            node_voltages_prev_step[groundNode->name] = 0.0;
        }

        int numUnknownNodeVoltages = current_matrix_index;
        int numFloatingVSBranchCurrents = floatingVSources.size();
        int numInductorBranchCurrents = inductors_in_circuit.size();
        int N_matrix_size = numUnknownNodeVoltages + numFloatingVSBranchCurrents + numInductorBranchCurrents;

        std::vector<double> Sol_V_unknown_prev(numUnknownNodeVoltages, 0.0);
        std::vector<double> Sol_I_L_prev(numInductorBranchCurrents, 0.0);
        std::vector<double> CurrentSolutionVector(N_matrix_size, 0.0);


        for(const auto& pair : initialFixedNodeVoltages_dc) {
            const std::string& name = pair.first;
            const double& val_dc = pair.second;
        }

        for (const auto& pair : initialFixedNodeVoltages_dc) {
            const std::string& name = pair.first;
            node_voltages_prev_step[name] = 0.0;
        }
        if (groundNode) {
            node_voltages_prev_step[groundNode->name] = 0.0;
        }


        std::cout << std::fixed << std::setprecision(5);

        for (double t_current = 0.0; t_current <= t_end + 1e-9; t_current += time_step_h) {
            std::vector<std::vector<double>> A_matrix(N_matrix_size, std::vector<double>(N_matrix_size, 0.0));
            std::vector<double> Z_vector(N_matrix_size, 0.0);

            fixedNodeVoltageAtT.clear();
            for (CircuitElement* el_fixed_update : elements) {
                if (VoltageSource* v_fixed_update = dynamic_cast<VoltageSource*>(el_fixed_update)) {
                    if (initialFixedNodeVoltages_dc.count(v_fixed_update->node1->name) && v_fixed_update->node2->isGround) {
                        fixedNodeVoltageAtT[v_fixed_update->node1->name] = v_fixed_update->getVoltageAtTime(t_current);
                    } else if (initialFixedNodeVoltages_dc.count(v_fixed_update->node2->name) && v_fixed_update->node1->isGround) {
                        fixedNodeVoltageAtT[v_fixed_update->node2->name] = -v_fixed_update->getVoltageAtTime(t_current);
                    }
                }
            }
            if(groundNode) fixedNodeVoltageAtT[groundNode->name] = 0.0;

            for (Diode* d_el : diodes_in_circuit) {
                if (node_voltages_prev_step.find(d_el->node1->name) == node_voltages_prev_step.end() ||
                    node_voltages_prev_step.find(d_el->node2->name) == node_voltages_prev_step.end()) {

                    double v1_prev_d_safe = node_voltages_prev_step.count(d_el->node1->name) ? node_voltages_prev_step.at(d_el->node1->name) : 0.0;
                    double v2_prev_d_safe = node_voltages_prev_step.count(d_el->node2->name) ? node_voltages_prev_step.at(d_el->node2->name) : 0.0;
                    d_el->updateOperatingState(v1_prev_d_safe, v2_prev_d_safe);
                } else {
                    d_el->updateOperatingState(node_voltages_prev_step.at(d_el->node1->name),
                                               node_voltages_prev_step.at(d_el->node2->name));
                }
            }
            for (CircuitElement* el : elements) {
                bool n1_is_fixed = fixedNodeVoltageAtT.count(el->node1->name);
                bool n2_is_fixed = fixedNodeVoltageAtT.count(el->node2->name);
                double v1_val_at_t = n1_is_fixed ? fixedNodeVoltageAtT.at(el->node1->name) : 0.0;
                double v2_val_at_t = n2_is_fixed ? fixedNodeVoltageAtT.at(el->node2->name) : 0.0;

                int idx_n1_unknown = unknownNodeMap.count(el->node1->name) ? unknownNodeMap.at(el->node1->name) : -1;
                int idx_n2_unknown = unknownNodeMap.count(el->node2->name) ? unknownNodeMap.at(el->node2->name) : -1;

                if (Resistor* r = dynamic_cast<Resistor*>(el)) {
                    double g = 1.0 / r->resistance;
                    if (idx_n1_unknown != -1) A_matrix[idx_n1_unknown][idx_n1_unknown] += g;
                    if (idx_n2_unknown != -1) A_matrix[idx_n2_unknown][idx_n2_unknown] += g;
                    if (idx_n1_unknown != -1 && idx_n2_unknown != -1) { A_matrix[idx_n1_unknown][idx_n2_unknown] -= g; A_matrix[idx_n2_unknown][idx_n1_unknown] -= g; }
                    if (idx_n1_unknown != -1 && n2_is_fixed) Z_vector[idx_n1_unknown] += g * v2_val_at_t;
                    if (idx_n2_unknown != -1 && n1_is_fixed) Z_vector[idx_n2_unknown] += g * v1_val_at_t;
                }
                else if (Capacitor* c = dynamic_cast<Capacitor*>(el)) {
                    double G_eq = c->capacitance / time_step_h;
                    if (idx_n1_unknown != -1) A_matrix[idx_n1_unknown][idx_n1_unknown] += G_eq;
                    if (idx_n2_unknown != -1) A_matrix[idx_n2_unknown][idx_n2_unknown] += G_eq;
                    if (idx_n1_unknown != -1 && idx_n2_unknown != -1) { A_matrix[idx_n1_unknown][idx_n2_unknown] -= G_eq; A_matrix[idx_n2_unknown][idx_n1_unknown] -= G_eq; }
                    if (idx_n1_unknown != -1 && n2_is_fixed) Z_vector[idx_n1_unknown] += G_eq * v2_val_at_t;
                    if (idx_n2_unknown != -1 && n1_is_fixed) Z_vector[idx_n2_unknown] += G_eq * v1_val_at_t;

                    double v1_prev_for_hist = node_voltages_prev_step.at(el->node1->name);
                    double v2_prev_for_hist = node_voltages_prev_step.at(el->node2->name);

                    double Vc_prev = v1_prev_for_hist - v2_prev_for_hist;
                    double I_hist = G_eq * Vc_prev;

                    if (idx_n1_unknown != -1) Z_vector[idx_n1_unknown] += I_hist;
                    if (idx_n2_unknown != -1) Z_vector[idx_n2_unknown] -= I_hist;
                }
                else if (Diode* d = dynamic_cast<Diode*>(el)) {
                    double G_eff = 0.0;
                    double I_eq_source = 0.0;

                    switch (d->current_op_state) {
                        case Diode::OperatingState::FORWARD_BIASED:
                            G_eff = d->G_on;

                            I_eq_source = G_eff * d->forward_voltage_Vf;
                            break;
                        case Diode::OperatingState::ZENER_BREAKDOWN:
                            G_eff = d->G_on;
                            I_eq_source = G_eff * (-d->zener_voltage_Vz);
                            break;
                        case Diode::OperatingState::OFF:
                        default:
                            G_eff = d->G_off;
                            I_eq_source = 0.0;
                            break;
                    }

                    if (idx_n1_unknown != -1) A_matrix[idx_n1_unknown][idx_n1_unknown] += G_eff;
                    if (idx_n2_unknown != -1) A_matrix[idx_n2_unknown][idx_n2_unknown] += G_eff;
                    if (idx_n1_unknown != -1 && idx_n2_unknown != -1) {
                        A_matrix[idx_n1_unknown][idx_n2_unknown] -= G_eff;
                        A_matrix[idx_n2_unknown][idx_n1_unknown] -= G_eff;
                    }
                    if (idx_n1_unknown != -1 && n2_is_fixed) Z_vector[idx_n1_unknown] += G_eff * v2_val_at_t;
                    if (idx_n2_unknown != -1 && n1_is_fixed) Z_vector[idx_n2_unknown] += G_eff * v1_val_at_t;


                    if (idx_n1_unknown != -1) Z_vector[idx_n1_unknown] -= I_eq_source;
                    if (idx_n2_unknown != -1) Z_vector[idx_n2_unknown] += I_eq_source;

                }
                else if (CurrentSource* is = dynamic_cast<CurrentSource*>(el)) {
                    double val = is->getCurrentAtTime(t_current);
                    if (idx_n1_unknown != -1) Z_vector[idx_n1_unknown] -= val;
                    if (idx_n2_unknown != -1) Z_vector[idx_n2_unknown] += val;
                }
            }

            for (int k_vs_idx = 0; k_vs_idx < numFloatingVSBranchCurrents; ++k_vs_idx) {
                VoltageSource* v_source = floatingVSources[k_vs_idx];
                int branch_eq_idx = numUnknownNodeVoltages + k_vs_idx;
                int n1_mat_idx = unknownNodeMap.count(v_source->node1->name) ? unknownNodeMap.at(v_source->node1->name) : -1;
                int n2_mat_idx = unknownNodeMap.count(v_source->node2->name) ? unknownNodeMap.at(v_source->node2->name) : -1;
                if (n1_mat_idx != -1) { A_matrix[n1_mat_idx][branch_eq_idx] += 1.0; A_matrix[branch_eq_idx][n1_mat_idx] += 1.0; }
                if (n2_mat_idx != -1) { A_matrix[n2_mat_idx][branch_eq_idx] -= 1.0; A_matrix[branch_eq_idx][n2_mat_idx] -= 1.0; }
                Z_vector[branch_eq_idx] += v_source->getVoltageAtTime(t_current);
                if (fixedNodeVoltageAtT.count(v_source->node1->name)) Z_vector[branch_eq_idx] -= fixedNodeVoltageAtT.at(v_source->node1->name);
                if (fixedNodeVoltageAtT.count(v_source->node2->name)) Z_vector[branch_eq_idx] += fixedNodeVoltageAtT.at(v_source->node2->name);
                v_source->current_MNA_solution = 0;
            }

            for (int k_L_idx = 0; k_L_idx < numInductorBranchCurrents; ++k_L_idx) {
                Inductor* ind = inductors_in_circuit[k_L_idx];
                int branch_eq_idx = numUnknownNodeVoltages + numFloatingVSBranchCurrents + k_L_idx;
                int n1_mat_idx = unknownNodeMap.count(ind->node1->name) ? unknownNodeMap.at(ind->node1->name) : -1;
                int n2_mat_idx = unknownNodeMap.count(ind->node2->name) ? unknownNodeMap.at(ind->node2->name) : -1;
                double R_L_companion = ind->inductance / time_step_h;
                double V_L_history = -R_L_companion * Sol_I_L_prev[k_L_idx];
                if (n1_mat_idx != -1) A_matrix[n1_mat_idx][branch_eq_idx] += 1.0;
                if (n2_mat_idx != -1) A_matrix[n2_mat_idx][branch_eq_idx] -= 1.0;
                if (n1_mat_idx != -1) A_matrix[branch_eq_idx][n1_mat_idx] += 1.0;
                if (n2_mat_idx != -1) A_matrix[branch_eq_idx][n2_mat_idx] -= 1.0;
                A_matrix[branch_eq_idx][branch_eq_idx] -= R_L_companion;
                Z_vector[branch_eq_idx] += V_L_history;
                if (fixedNodeVoltageAtT.count(ind->node1->name)) Z_vector[branch_eq_idx] -= fixedNodeVoltageAtT.at(ind->node1->name);
                if (fixedNodeVoltageAtT.count(ind->node2->name)) Z_vector[branch_eq_idx] += fixedNodeVoltageAtT.at(ind->node2->name);
                ind->current_MNA_solution = 0;
            }

            if (N_matrix_size > 0) {
                for (int k_piv = 0; k_piv < N_matrix_size; ++k_piv) {
                    int piv_r = k_piv; double max_v = std::abs(A_matrix[k_piv][k_piv]);
                    for (int r_it = k_piv + 1; r_it < N_matrix_size; ++r_it) if (std::abs(A_matrix[r_it][k_piv]) > max_v) { max_v = std::abs(A_matrix[r_it][k_piv]); piv_r = r_it; }
                    if (k_piv != piv_r) { std::swap(A_matrix[k_piv], A_matrix[piv_r]); std::swap(Z_vector[k_piv], Z_vector[piv_r]); }
                    double piv_val = A_matrix[k_piv][k_piv];
                    if (std::abs(piv_val) < 1e-12) { for(auto& val_s : CurrentSolutionVector) val_s = std::numeric_limits<double>::quiet_NaN(); break; }
                    for (int j_c = k_piv; j_c < N_matrix_size; ++j_c) A_matrix[k_piv][j_c] /= piv_val;
                    Z_vector[k_piv] /= piv_val;
                    for (int i_r = 0; i_r < N_matrix_size; ++i_r) {
                        if (i_r != k_piv) {
                            double fact = A_matrix[i_r][k_piv];
                            for (int j_c = k_piv; j_c < N_matrix_size; ++j_c) A_matrix[i_r][j_c] -= fact * A_matrix[k_piv][j_c];
                            Z_vector[i_r] -= fact * Z_vector[k_piv];
                        }
                    }
                }
                if (!std::isnan(CurrentSolutionVector[0]) || N_matrix_size == 0) { CurrentSolutionVector = Z_vector; }
            }

            for (int k_vs_idx = 0; k_vs_idx < numFloatingVSBranchCurrents; ++k_vs_idx) {
                if (numUnknownNodeVoltages + k_vs_idx < CurrentSolutionVector.size())
                    floatingVSources[k_vs_idx]->current_MNA_solution = CurrentSolutionVector[numUnknownNodeVoltages + k_vs_idx];
            }
            for (int k_L_idx = 0; k_L_idx < numInductorBranchCurrents; ++k_L_idx) {
                if (numUnknownNodeVoltages + numFloatingVSBranchCurrents + k_L_idx < CurrentSolutionVector.size())
                    inductors_in_circuit[k_L_idx]->current_MNA_solution = CurrentSolutionVector[numUnknownNodeVoltages + numFloatingVSBranchCurrents + k_L_idx];
            }

            std::cout << "t = " << t_current << " s";
            std::map<std::string, double> current_node_voltages_for_print;

            for (const auto& pair : fixedNodeVoltageAtT) {
                const std::string& name = pair.first;
                const double& val_at_t = pair.second;
                if (groundNode && name == groundNode->name) {} // Don't print V(GND) here yet
                else std::cout << ", V(" << name << ") = " << val_at_t << " V";
                current_node_voltages_for_print[name] = val_at_t;
            }
            for (const auto& pair : unknownNodeMap) {
                const std::string& name = pair.first;
                const int& matrix_map_idx = pair.second;
                if (matrix_map_idx < numUnknownNodeVoltages && matrix_map_idx < CurrentSolutionVector.size()) {
                    std::cout << ", V(" << name << ") = " << CurrentSolutionVector[matrix_map_idx] << " V";
                    current_node_voltages_for_print[name] = CurrentSolutionVector[matrix_map_idx];
                }
            }
            std::map<std::string, double> element_currents_map;


            for (CircuitElement* el_curr : elements) {
                double v1_now_p = current_node_voltages_for_print.at(el_curr->node1->name);
                double v2_now_p = current_node_voltages_for_print.at(el_curr->node2->name);
                double v1_prev_p = node_voltages_prev_step.at(el_curr->node1->name);
                double v2_prev_p = node_voltages_prev_step.at(el_curr->node2->name);
                double current_val_calc = 0;

                if (Resistor* r = dynamic_cast<Resistor*>(el_curr)) {
                    current_val_calc = r->getCurrent(v1_now_p, v2_now_p, v1_prev_p, v2_prev_p, time_step_h, t_current);
                } else if (Capacitor* c_el = dynamic_cast<Capacitor*>(el_curr)) {
                    current_val_calc = c_el->getCurrent(v1_now_p, v2_now_p, v1_prev_p, v2_prev_p, time_step_h, t_current);
                } else if (Inductor* l_el = dynamic_cast<Inductor*>(el_curr)) {
                    current_val_calc = l_el->getCurrent(0,0,0,0,0, t_current);
                } else if (VoltageSource* vs_el = dynamic_cast<VoltageSource*>(el_curr)) {
                    bool is_floating = (std::find(floatingVSources.begin(), floatingVSources.end(), vs_el) != floatingVSources.end());
                    if (is_floating) {
                        current_val_calc = vs_el->getCurrent(0,0,0,0,0, t_current);
                    } else {
                        current_val_calc = 0.0;
                    }
                }
                else if (CurrentSource* cs_el = dynamic_cast<CurrentSource*>(el_curr)) {
                    current_val_calc = cs_el->getCurrent(0,0,0,0,0, t_current);
                }
                else if (Diode* d_el = dynamic_cast<Diode*>(el_curr)) {
                    current_val_calc = d_el->getCurrent(v1_now_p, v2_now_p, v1_prev_p, v2_prev_p, time_step_h, t_current);
                }
                element_currents_map[el_curr->name] = current_val_calc;
            }

            for (CircuitElement* el_vs_kcl : elements) {
                if (VoltageSource* vs_kcl = dynamic_cast<VoltageSource*>(el_vs_kcl)) {
                    bool is_actually_floating = false;
                    for(const auto& fvs_ptr : floatingVSources) if(fvs_ptr == vs_kcl) {is_actually_floating = true; break;}

                    if (!is_actually_floating) {
                        double sum_currents_out_of_pos_node = 0;
                        Node* positive_terminal_node = nullptr;
                        Node* negative_terminal_node = nullptr;

                        if (vs_kcl->node2->isGround && initialFixedNodeVoltages_dc.count(vs_kcl->node1->name)) {
                            positive_terminal_node = vs_kcl->node1; // V(N1, GND)
                            negative_terminal_node = vs_kcl->node2;
                        } else if (vs_kcl->node1->isGround && initialFixedNodeVoltages_dc.count(vs_kcl->node2->name)) {
                            positive_terminal_node = vs_kcl->node2; // V(GND, N2) => N2 is positive
                            negative_terminal_node = vs_kcl->node1;
                        }

                        if (positive_terminal_node) {
                            for (CircuitElement* connected_el : elements) {
                                if (connected_el == vs_kcl) continue;

                                if (connected_el->node1 == positive_terminal_node) {
                                    sum_currents_out_of_pos_node += element_currents_map.at(connected_el->name);
                                } else if (connected_el->node2 == positive_terminal_node) {
                                    sum_currents_out_of_pos_node -= element_currents_map.at(connected_el->name);
                                }
                            }

                            element_currents_map[vs_kcl->name] = sum_currents_out_of_pos_node;
                        }
                    }
                }
            }

            for (const auto& pair_curr : element_currents_map) {
                std::cout << ", I(" << pair_curr.first << ") = " << pair_curr.second << " A";
            }
            std::cout << std::endl;


            for(auto& pair_node_prev_v : node_voltages_prev_step){
                const std::string& node_name = pair_node_prev_v.first;
                if(current_node_voltages_for_print.count(node_name)) { // Use the map built for printing
                    pair_node_prev_v.second = current_node_voltages_for_print.at(node_name);
                }
            }

            for (int i_v_node_idx = 0; i_v_node_idx < numUnknownNodeVoltages; ++i_v_node_idx) {
                if (i_v_node_idx < CurrentSolutionVector.size())
                    Sol_V_unknown_prev[i_v_node_idx] = CurrentSolutionVector[i_v_node_idx];
            }
            for (int k_L_branch_idx = 0; k_L_branch_idx < numInductorBranchCurrents; ++k_L_branch_idx) {
                int sol_vec_idx = numUnknownNodeVoltages + numFloatingVSBranchCurrents + k_L_branch_idx;
                if (sol_vec_idx < CurrentSolutionVector.size())
                    Sol_I_L_prev[k_L_branch_idx] = CurrentSolutionVector[sol_vec_idx];
            }
        }
    }
    struct DCOpPointSolution {
        bool converged = false;
        std::map<std::string, double> nodeVoltages;
        std::map<std::string, double> elementCurrents;

        DCOpPointSolution() : converged(false) {}
    };
    DCOpPointSolution solveDCOperatingPoint(int max_iterations = 10, double tolerance = 1e-6);
    void performDCSweep(const std::string& sourceName, double startVal, double endVal, double increment);

};
Schematic::DCOpPointSolution Schematic::solveDCOperatingPoint(int max_iterations, double tolerance) {
    DCOpPointSolution solution;

    std::map<std::string, double> fixedNodeVoltages_DC;
    std::vector<VoltageSource*> floatingVSources_DC;
    std::vector<Inductor*> inductors_in_circuit_DC;
    std::vector<Diode*> diodes_in_circuit_DC;

    for (CircuitElement* el : elements) {
        if (VoltageSource* v = dynamic_cast<VoltageSource*>(el)) {

            double v_dc_val = v->getVoltageAtTime(0.0);
            if (v->node2->isGround) {
                fixedNodeVoltages_DC[v->node1->name] = v_dc_val;
            } else if (v->node1->isGround) {
                fixedNodeVoltages_DC[v->node2->name] = -v_dc_val;
            } else {
                floatingVSources_DC.push_back(v);
            }
        } else if (Inductor* l = dynamic_cast<Inductor*>(el)) {

            inductors_in_circuit_DC.push_back(l);
        } else if (Diode* d = dynamic_cast<Diode*>(el)) {
            diodes_in_circuit_DC.push_back(d);
        }

    }
    if (groundNode) {
        fixedNodeVoltages_DC[groundNode->name] = 0.0;
    }

    std::map<std::string, int> unknownNodeMap_DC;
    int current_matrix_index_DC = 0;
    for (Node* node : nodes) {
        if (!node->isGround && fixedNodeVoltages_DC.find(node->name) == fixedNodeVoltages_DC.end()) {
            unknownNodeMap_DC[node->name] = current_matrix_index_DC++;
        }
    }

    int numUnknownNodeVoltages_DC = current_matrix_index_DC;
    int numFloatingVSBranchCurrents_DC = floatingVSources_DC.size();
    int numInductorBranchCurrents_DC = inductors_in_circuit_DC.size();
    int N_matrix_size_DC = numUnknownNodeVoltages_DC + numFloatingVSBranchCurrents_DC + numInductorBranchCurrents_DC;

    // --- Handle trivial case: no unknowns ---
    if (N_matrix_size_DC == 0) {
        solution.nodeVoltages = fixedNodeVoltages_DC;
        if(groundNode && !solution.nodeVoltages.count(groundNode->name)){
            solution.nodeVoltages[groundNode->name] = 0.0;
        }
        solution.converged = true;


        for (CircuitElement* el : elements) {
            double v1 = solution.nodeVoltages.count(el->node1->name) ? solution.nodeVoltages.at(el->node1->name) : 0.0;
            double v2 = solution.nodeVoltages.count(el->node2->name) ? solution.nodeVoltages.at(el->node2->name) : 0.0;

            if (Resistor* r = dynamic_cast<Resistor*>(el)) {
                solution.elementCurrents[r->name] = (r->resistance == 0) ? std::numeric_limits<double>::infinity() : (v1 - v2) / r->resistance;
            } else if (dynamic_cast<Capacitor*>(el)) {
                solution.elementCurrents[el->name] = 0.0; // Open in DC
            } else if (CurrentSource* is = dynamic_cast<CurrentSource*>(el)){
                solution.elementCurrents[is->name] = is->getCurrentAtTime(0.0); // DC value
            } else if (VoltageSource* vs = dynamic_cast<VoltageSource*>(el)){

                solution.elementCurrents[vs->name] = 0.0;
            } else if (dynamic_cast<Inductor*>(el)){

                solution.elementCurrents[el->name] = 0.0;
            } else if (Diode* d = dynamic_cast<Diode*>(el)) {
                d->updateOperatingState(v1, v2);
                solution.elementCurrents[d->name] = d->getCurrent(v1, v2, v1, v2, 0, 0); // h=0, t=0 for DC
            }
        }

        node_voltages_prev_step = solution.nodeVoltages;
        return solution;
    }

    std::map<std::string, double> node_voltages_iter_guess = node_voltages_prev_step;
    for(Node* n_iter_init : nodes){
        if(fixedNodeVoltages_DC.count(n_iter_init->name)){
            node_voltages_iter_guess[n_iter_init->name] = fixedNodeVoltages_DC.at(n_iter_init->name);
        } else if(node_voltages_iter_guess.find(n_iter_init->name) == node_voltages_iter_guess.end()){
            node_voltages_iter_guess[n_iter_init->name] = 0.0;
        }
    }
    if (groundNode) node_voltages_iter_guess[groundNode->name] = 0.0;


    for (int iter = 0; iter < max_iterations; ++iter) {
        std::vector<std::vector<double>> A_matrix_DC(N_matrix_size_DC, std::vector<double>(N_matrix_size_DC, 0.0));
        std::vector<double> Z_vector_DC(N_matrix_size_DC, 0.0);
        std::vector<double> CurrentSolutionVector_DC(N_matrix_size_DC, 0.0);

        for (Diode* d_el : diodes_in_circuit_DC) {
            double v1_guess = node_voltages_iter_guess.count(d_el->node1->name) ? node_voltages_iter_guess.at(d_el->node1->name) : 0.0;
            double v2_guess = node_voltages_iter_guess.count(d_el->node2->name) ? node_voltages_iter_guess.at(d_el->node2->name) : 0.0;
            d_el->updateOperatingState(v1_guess, v2_guess);
        }


        for (CircuitElement* el : elements) {
            bool n1_is_fixed_dc = fixedNodeVoltages_DC.count(el->node1->name);
            bool n2_is_fixed_dc = fixedNodeVoltages_DC.count(el->node2->name);
            double v1_val_fixed_dc = n1_is_fixed_dc ? fixedNodeVoltages_DC.at(el->node1->name) : 0.0;
            double v2_val_fixed_dc = n2_is_fixed_dc ? fixedNodeVoltages_DC.at(el->node2->name) : 0.0;
            int idx_n1_unknown_dc = unknownNodeMap_DC.count(el->node1->name) ? unknownNodeMap_DC.at(el->node1->name) : -1;
            int idx_n2_unknown_dc = unknownNodeMap_DC.count(el->node2->name) ? unknownNodeMap_DC.at(el->node2->name) : -1;

            if (Resistor* r = dynamic_cast<Resistor*>(el)) {
                double g = (r->resistance == 0) ? 1.0/1e-12 : 1.0 / r->resistance;
                if (idx_n1_unknown_dc != -1) A_matrix_DC[idx_n1_unknown_dc][idx_n1_unknown_dc] += g;
                if (idx_n2_unknown_dc != -1) A_matrix_DC[idx_n2_unknown_dc][idx_n2_unknown_dc] += g;
                if (idx_n1_unknown_dc != -1 && idx_n2_unknown_dc != -1) { A_matrix_DC[idx_n1_unknown_dc][idx_n2_unknown_dc] -= g; A_matrix_DC[idx_n2_unknown_dc][idx_n1_unknown_dc] -= g; }
                if (idx_n1_unknown_dc != -1 && n2_is_fixed_dc) Z_vector_DC[idx_n1_unknown_dc] += g * v2_val_fixed_dc;
                if (idx_n2_unknown_dc != -1 && n1_is_fixed_dc) Z_vector_DC[idx_n2_unknown_dc] += g * v1_val_fixed_dc;
            } else if (dynamic_cast<Capacitor*>(el)) {

            } else if (Diode* d = dynamic_cast<Diode*>(el)) {
                double G_eff = 0.0; double I_eq_source = 0.0;
                switch (d->current_op_state) {
                    case Diode::OperatingState::FORWARD_BIASED: G_eff = d->G_on; I_eq_source = G_eff * d->forward_voltage_Vf; break;
                    case Diode::OperatingState::ZENER_BREAKDOWN: G_eff = d->G_on; I_eq_source = G_eff * (-d->zener_voltage_Vz); break;
                    case Diode::OperatingState::OFF: default:    G_eff = d->G_off; I_eq_source = 0.0; break;
                }

                if (idx_n1_unknown_dc != -1) A_matrix_DC[idx_n1_unknown_dc][idx_n1_unknown_dc] += G_eff;
                if (idx_n2_unknown_dc != -1) A_matrix_DC[idx_n2_unknown_dc][idx_n2_unknown_dc] += G_eff;
                if (idx_n1_unknown_dc != -1 && idx_n2_unknown_dc != -1) { A_matrix_DC[idx_n1_unknown_dc][idx_n2_unknown_dc] -= G_eff; A_matrix_DC[idx_n2_unknown_dc][idx_n1_unknown_dc] -= G_eff; }
                if (idx_n1_unknown_dc != -1 && n2_is_fixed_dc) Z_vector_DC[idx_n1_unknown_dc] += G_eff * v2_val_fixed_dc;
                if (idx_n2_unknown_dc != -1 && n1_is_fixed_dc) Z_vector_DC[idx_n2_unknown_dc] += G_eff * v1_val_fixed_dc;

                if (idx_n1_unknown_dc != -1) Z_vector_DC[idx_n1_unknown_dc] -= I_eq_source;
                if (idx_n2_unknown_dc != -1) Z_vector_DC[idx_n2_unknown_dc] += I_eq_source;
            } else if (CurrentSource* is = dynamic_cast<CurrentSource*>(el)) {
                double val = is->getCurrentAtTime(0.0);
                if (idx_n1_unknown_dc != -1) Z_vector_DC[idx_n1_unknown_dc] -= val;
                if (idx_n2_unknown_dc != -1) Z_vector_DC[idx_n2_unknown_dc] += val;
            }
        }


        for (int k_vs_idx = 0; k_vs_idx < numFloatingVSBranchCurrents_DC; ++k_vs_idx) {
            VoltageSource* v_source = floatingVSources_DC[k_vs_idx];
            int branch_eq_idx = numUnknownNodeVoltages_DC + k_vs_idx;
            int n1_mat_idx = unknownNodeMap_DC.count(v_source->node1->name) ? unknownNodeMap_DC.at(v_source->node1->name) : -1;
            int n2_mat_idx = unknownNodeMap_DC.count(v_source->node2->name) ? unknownNodeMap_DC.at(v_source->node2->name) : -1;


            if (n1_mat_idx != -1) A_matrix_DC[n1_mat_idx][branch_eq_idx] += 1.0;
            if (n2_mat_idx != -1) A_matrix_DC[n2_mat_idx][branch_eq_idx] -= 1.0;


            if (n1_mat_idx != -1) A_matrix_DC[branch_eq_idx][n1_mat_idx] += 1.0;
            if (n2_mat_idx != -1) A_matrix_DC[branch_eq_idx][n2_mat_idx] -= 1.0;
            Z_vector_DC[branch_eq_idx] += v_source->getVoltageAtTime(0.0);

        }


        for (int k_L_idx = 0; k_L_idx < numInductorBranchCurrents_DC; ++k_L_idx) {
            Inductor* ind = inductors_in_circuit_DC[k_L_idx];
            int branch_eq_idx = numUnknownNodeVoltages_DC + numFloatingVSBranchCurrents_DC + k_L_idx;
            int n1_mat_idx = unknownNodeMap_DC.count(ind->node1->name) ? unknownNodeMap_DC.at(ind->node1->name) : -1;
            int n2_mat_idx = unknownNodeMap_DC.count(ind->node2->name) ? unknownNodeMap_DC.at(ind->node2->name) : -1;


            if (n1_mat_idx != -1) A_matrix_DC[n1_mat_idx][branch_eq_idx] += 1.0;
            if (n2_mat_idx != -1) A_matrix_DC[n2_mat_idx][branch_eq_idx] -= 1.0;


            if (n1_mat_idx != -1) A_matrix_DC[branch_eq_idx][n1_mat_idx] += 1.0;
            if (n2_mat_idx != -1) A_matrix_DC[branch_eq_idx][n2_mat_idx] -= 1.0;
            Z_vector_DC[branch_eq_idx] = 0.0;
        }


        bool singular = false;
        if (N_matrix_size_DC > 0) {
            std::vector<std::vector<double>> A_copy = A_matrix_DC;
            std::vector<double> Z_copy = Z_vector_DC;

            for (int k_piv = 0; k_piv < N_matrix_size_DC; ++k_piv) {
                int piv_r = k_piv; double max_v = std::abs(A_copy[k_piv][k_piv]);
                for (int r_it = k_piv + 1; r_it < N_matrix_size_DC; ++r_it) if (std::abs(A_copy[r_it][k_piv]) > max_v) { max_v = std::abs(A_copy[r_it][k_piv]); piv_r = r_it; }
                if (k_piv != piv_r) { std::swap(A_copy[k_piv], A_copy[piv_r]); std::swap(Z_copy[k_piv], Z_copy[piv_r]); }
                double piv_val = A_copy[k_piv][k_piv];
                if (std::abs(piv_val) < 1e-12) { singular = true; break; }
                for (int j_c = k_piv; j_c < N_matrix_size_DC; ++j_c) A_copy[k_piv][j_c] /= piv_val;
                Z_copy[k_piv] /= piv_val;
                for (int i_r = 0; i_r < N_matrix_size_DC; ++i_r) {
                    if (i_r != k_piv) {
                        double fact = A_copy[i_r][k_piv];
                        for (int j_c = k_piv; j_c < N_matrix_size_DC; ++j_c) A_copy[i_r][j_c] -= fact * A_copy[k_piv][j_c];
                        Z_copy[i_r] -= fact * Z_copy[k_piv];
                    }
                }
            }
            if (!singular) CurrentSolutionVector_DC = Z_copy;
        }


        if (singular) {
            std::cerr << "Warning: DC MNA matrix singular at iteration " << iter +1 << ". Solution may be invalid." << std::endl;
            solution.converged = false;
            for(const auto& pair : unknownNodeMap_DC) {
                solution.nodeVoltages[pair.first] = std::numeric_limits<double>::quiet_NaN();
            }
            for(const auto& pair : fixedNodeVoltages_DC){
                solution.nodeVoltages[pair.first] = pair.second;
            }
            if(groundNode) solution.nodeVoltages[groundNode->name] = 0.0;
            return solution;
        }


        bool iteration_converged = true;
        if (diodes_in_circuit_DC.empty() && iter == 0) {
            iteration_converged = true;
        } else if (!diodes_in_circuit_DC.empty()) {
            std::map<std::string, double> current_iter_voltages_full_map = fixedNodeVoltages_DC;
            for(const auto& pair : unknownNodeMap_DC) {
                if (pair.second < numUnknownNodeVoltages_DC && pair.second < CurrentSolutionVector_DC.size())
                    current_iter_voltages_full_map[pair.first] = CurrentSolutionVector_DC[pair.second];
            }
            if(groundNode && !current_iter_voltages_full_map.count(groundNode->name)){
                current_iter_voltages_full_map[groundNode->name] = 0.0;
            }

            for (Node* n_check : nodes) {
                double v_new = current_iter_voltages_full_map.count(n_check->name) ? current_iter_voltages_full_map.at(n_check->name) : 0.0;
                double v_old_guess = node_voltages_iter_guess.count(n_check->name) ? node_voltages_iter_guess.at(n_check->name) : 0.0;
                if (std::abs(v_new - v_old_guess) > tolerance) {
                    iteration_converged = false;
                    break;
                }
            }
            node_voltages_iter_guess = current_iter_voltages_full_map;
        }


        if (iteration_converged) {
            solution.converged = true;
            solution.nodeVoltages = fixedNodeVoltages_DC;
            for(const auto& pair : unknownNodeMap_DC) {
                if (pair.second < numUnknownNodeVoltages_DC && pair.second < CurrentSolutionVector_DC.size())
                    solution.nodeVoltages[pair.first] = CurrentSolutionVector_DC[pair.second];
            }
            if(groundNode && !solution.nodeVoltages.count(groundNode->name)){
                solution.nodeVoltages[groundNode->name] = 0.0;
            }



            for (int k_vs_idx = 0; k_vs_idx < numFloatingVSBranchCurrents_DC; ++k_vs_idx) {
                int sol_idx = numUnknownNodeVoltages_DC + k_vs_idx;
                if (sol_idx < CurrentSolutionVector_DC.size()){
                    floatingVSources_DC[k_vs_idx]->current_MNA_solution = CurrentSolutionVector_DC[sol_idx];
                    solution.elementCurrents[floatingVSources_DC[k_vs_idx]->name] = CurrentSolutionVector_DC[sol_idx];
                }
            }
            for (int k_L_idx = 0; k_L_idx < numInductorBranchCurrents_DC; ++k_L_idx) {
                int sol_idx = numUnknownNodeVoltages_DC + numFloatingVSBranchCurrents_DC + k_L_idx;
                if (sol_idx < CurrentSolutionVector_DC.size()){
                    inductors_in_circuit_DC[k_L_idx]->current_MNA_solution = CurrentSolutionVector_DC[sol_idx];
                    solution.elementCurrents[inductors_in_circuit_DC[k_L_idx]->name] = CurrentSolutionVector_DC[sol_idx];
                }
            }
            node_voltages_prev_step = solution.nodeVoltages;
            break;
        } else if (iter == max_iterations - 1) {
            std::cerr << "Warning: DC operating point iterations did not converge after " << max_iterations << " iterations." << std::endl;
            solution.converged = false;
            solution.nodeVoltages = node_voltages_iter_guess;
        }
    }

    for (CircuitElement* el : elements) {
        if (solution.elementCurrents.count(el->name)) continue;


        double v1 = solution.nodeVoltages.count(el->node1->name) ? solution.nodeVoltages.at(el->node1->name) : 0.0;
        double v2 = solution.nodeVoltages.count(el->node2->name) ? solution.nodeVoltages.at(el->node2->name) : 0.0;
        if(std::isnan(v1) || std::isnan(v2)) {
            solution.elementCurrents[el->name] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }


        if (Resistor* r = dynamic_cast<Resistor*>(el)) {
            solution.elementCurrents[r->name] = (r->resistance == 0) ? ((v1==v2) ? 0.0 : std::numeric_limits<double>::infinity() * ((v1 > v2) ? 1 : -1) )
                                                                     : (v1 - v2) / r->resistance;
        } else if (dynamic_cast<Capacitor*>(el)) {
            solution.elementCurrents[el->name] = 0.0;
        } else if (CurrentSource* is = dynamic_cast<CurrentSource*>(el)){
            solution.elementCurrents[is->name] = is->getCurrentAtTime(0.0);
        } else if (Diode* d = dynamic_cast<Diode*>(el)) {
            d->updateOperatingState(v1, v2);

            solution.elementCurrents[d->name] = d->getCurrent(v1, v2, v1, v2, 0, 0);
        }
    }
    for (CircuitElement* el_vs_kcl : elements) {
        if (VoltageSource* vs_kcl = dynamic_cast<VoltageSource*>(el_vs_kcl)) {
            bool is_floating = false;
            for(const auto* fvs_ptr : floatingVSources_DC) if(fvs_ptr == vs_kcl) {is_floating = true; break;}

            if (!is_floating && (vs_kcl->node1->isGround || vs_kcl->node2->isGround) ) {
                double sum_currents_out_of_non_ground_node = 0;
                Node* non_ground_terminal = nullptr;


                bool n1_is_non_ground = !vs_kcl->node1->isGround && fixedNodeVoltages_DC.count(vs_kcl->node1->name);
                bool n2_is_non_ground = !vs_kcl->node2->isGround && fixedNodeVoltages_DC.count(vs_kcl->node2->name);
                if (n1_is_non_ground) non_ground_terminal = vs_kcl->node1;
                else if (n2_is_non_ground) non_ground_terminal = vs_kcl->node2;
                if (non_ground_terminal) {
                    for (CircuitElement* connected_el : elements) {
                        if (connected_el == vs_kcl) continue;


                        if (!solution.elementCurrents.count(connected_el->name)) {


                            std::cerr << "Warning: KCL for VSource " << vs_kcl->name
                                      << ": Current for connected element " << connected_el->name
                                      << " not found. Assuming 0A for KCL." << std::endl;
                            continue;
                        }
                        double current_val_conn = solution.elementCurrents.at(connected_el->name);
                        if(std::isnan(current_val_conn)) continue;

                        if (connected_el->node1 == non_ground_terminal) {
                            sum_currents_out_of_non_ground_node += current_val_conn;
                        } else if (connected_el->node2 == non_ground_terminal) {
                            sum_currents_out_of_non_ground_node -= current_val_conn;
                        }
                    }

                    if (non_ground_terminal == vs_kcl->node1) {
                        solution.elementCurrents[vs_kcl->name] = sum_currents_out_of_non_ground_node;
                    } else {
                        solution.elementCurrents[vs_kcl->name] = -sum_currents_out_of_non_ground_node;
                    }
                } else if (!is_floating) {



                    if(!solution.elementCurrents.count(vs_kcl->name)) {
                        solution.elementCurrents[vs_kcl->name] = std::numeric_limits<double>::quiet_NaN();
                    }
                }
            }
        }
    }
    return solution;
}
void Schematic::performDCSweep(const std::string& sourceName, double startVal, double endVal, double increment) {
    CircuitElement* sweptElement = findElement(sourceName);
    if (!sweptElement) {
        std::cout << "Error: Source '" << sourceName << "' not found for DC sweep." << std::endl;
        return;
    }

    VoltageSource* vs_swept = dynamic_cast<VoltageSource*>(sweptElement);
    CurrentSource* cs_swept = dynamic_cast<CurrentSource*>(sweptElement);

    if (!vs_swept && !cs_swept) {
        std::cout << "Error: Element '" << sourceName << "' is not a voltage or current source." << std::endl;
        return;
    }

    if (std::abs(increment) < 1e-12) {
        std::cout << "Error: DC sweep increment is too small or zero." << std::endl;
        return;
    }
    if ((endVal > startVal && increment < 0) || (endVal < startVal && increment > 0)) {
        std::cout << "Error: Increment sign will not allow sweep to reach end value." << std::endl;
        return;
    }
    if (startVal == endVal && increment !=0) {
        std::cout << "Warning: Start and End values for DC sweep are the same. Performing single point analysis." << std::endl;

    }


    double original_dc_val_vs = 0.0, original_amp_vs = 0.0, original_freq_vs = 0.0, original_phase_vs = 0.0;
    bool original_is_sin_vs = false;

    double original_dc_val_cs = 0.0, original_amp_cs = 0.0, original_freq_cs = 0.0, original_phase_cs = 0.0;
    bool original_is_sin_cs = false;


    if (vs_swept) {
        original_dc_val_vs = vs_swept->dc_voltage;
        original_is_sin_vs = vs_swept->isSinusoidal;
        original_amp_vs = vs_swept->amplitude;
        original_freq_vs = vs_swept->frequency;
        original_phase_vs = vs_swept->phase_degrees;
    } else if (cs_swept) {
        original_dc_val_cs = cs_swept->dc_current;
        original_is_sin_cs = cs_swept->isSinusoidal;
        original_amp_cs = cs_swept->amplitude;
        original_freq_cs = cs_swept->frequency;
        original_phase_cs = cs_swept->phase_degrees;
    }

    std::cout << std::fixed << std::setprecision(5);
    std::cout << "\nDC SWEEP RESULTS for " << sourceName << ":" << std::endl;
    std::vector<std::string> node_print_order;
    for(Node* n : nodes) {
        if (n != groundNode && !(n->isGround) ) {
            node_print_order.push_back(n->name);
        }
    }
    std::sort(node_print_order.begin(), node_print_order.end());
    std::vector<std::string> element_current_print_order;
    for(CircuitElement* el_p : elements) {
        element_current_print_order.push_back(el_p->name);
    }
    std::sort(element_current_print_order.begin(), element_current_print_order.end());
    std::cout << std::setw(10) << std::left << sourceName;
    for(const std::string& n_name : node_print_order) {
        std::cout << std::setw(12) << std::left << ("V(" + n_name + ")");
    }
    for(const std::string& el_name_p : element_current_print_order) {
        std::cout << std::setw(12) << std::left << ("I(" + el_name_p + ")");
    }
    std::cout << std::endl;
    std::cout << std::setw(10) << std::left << std::string(sourceName.length() > 0 ? sourceName.length() : 6, '-');
    for(size_t i = 0; i < node_print_order.size(); ++i) {
        std::cout << std::setw(12) << std::left << "-----------";
    }
    for(size_t i = 0; i < element_current_print_order.size(); ++i) {
        std::cout << std::setw(12) << std::left << "-----------";
    }
    std::cout << std::endl;
    for (Node* n : nodes) {
        node_voltages_prev_step[n->name] = 0.0;
    }
    if (groundNode) {
        node_voltages_prev_step[groundNode->name] = 0.0;
    }

    double sweepVal = startVal;
    bool forwardSweep = (endVal >= startVal);
    if (increment == 0 && startVal != endVal) {
        std::cout << "Error: Increment is zero but start and end values differ. Performing single point analysis at startVal." << std::endl;
        endVal = startVal;
    }


    int step_count = 0;
    const int MAX_SWEEP_STEPS = 100000;

    while (step_count < MAX_SWEEP_STEPS) {

        if (vs_swept) {
            vs_swept->dc_voltage = sweepVal;
            vs_swept->isSinusoidal = false;
            vs_swept->amplitude = 0;
            vs_swept->frequency = 0;
        } else if (cs_swept) {
            cs_swept->dc_current = sweepVal;
            cs_swept->isSinusoidal = false;
            cs_swept->amplitude = 0;
            cs_swept->frequency = 0;
        }


        DCOpPointSolution op_solution = solveDCOperatingPoint(30, 1e-6);

        std::cout << std::setw(10) << std::left << sweepVal;
        if (op_solution.converged) {
            for(const std::string& n_name : node_print_order) {
                double val_to_print = op_solution.nodeVoltages.count(n_name) ? op_solution.nodeVoltages.at(n_name) : 0.0;
                if (std::isnan(val_to_print)) std::cout << std::setw(12) << std::left << "NaN";
                else std::cout << std::setw(12) << std::left << val_to_print;
            }
            for(const std::string& el_name_p : element_current_print_order) {
                double val_to_print = op_solution.elementCurrents.count(el_name_p) ? op_solution.elementCurrents.at(el_name_p) : 0.0;
                if (std::isnan(val_to_print)) std::cout << std::setw(12) << std::left << "NaN";
                else if (std::isinf(val_to_print)) std::cout << std::setw(12) << std::left << (val_to_print > 0 ? "Inf" : "-Inf");
                else std::cout << std::setw(12) << std::left << val_to_print;
            }
        } else {
            std::cout << std::setw(12) << std::left << "(NC)";
            for(size_t i = 0; i < node_print_order.size() + element_current_print_order.size(); ++i) {
                std::cout << std::setw(12) << std::left << "N/A";
            }
        }
        std::cout << std::endl;

        if ((forwardSweep && sweepVal >= endVal - std::abs(increment) * 0.001) ||
            (!forwardSweep && sweepVal <= endVal + std::abs(increment) * 0.001) ||
            std::abs(sweepVal - endVal) < 1e-9 ||
            (increment == 0.0) ) {
            break;
        }

        sweepVal += increment;

        if ((forwardSweep && sweepVal > endVal && (sweepVal - increment < endVal - 1e-9)) ||
            (!forwardSweep && sweepVal < endVal && (sweepVal - increment > endVal + 1e-9))) {
            sweepVal = endVal;
        }
        step_count++;
    }

    if(step_count >= MAX_SWEEP_STEPS) {
        std::cout << "Warning: DC Sweep exceeded maximum steps (" << MAX_SWEEP_STEPS << ")." << std::endl;
    }


    if (vs_swept) {
        vs_swept->dc_voltage = original_dc_val_vs;
        vs_swept->isSinusoidal = original_is_sin_vs;
        vs_swept->amplitude = original_amp_vs;
        vs_swept->frequency = original_freq_vs;
        vs_swept->phase_degrees = original_phase_vs;
    } else if (cs_swept) {
        cs_swept->dc_current = original_dc_val_cs;
        cs_swept->isSinusoidal = original_is_sin_cs;
        cs_swept->amplitude = original_amp_cs;
        cs_swept->frequency = original_freq_cs;
        cs_swept->phase_degrees = original_phase_cs;
    }

    std::cout << "\nDC Sweep finished." << std::endl;
}

class CircuitSimulator {
private:
    std::vector<std::string> _getAvailableSchematicNamesForMenu() {
        std::map<std::string, bool> unique_names;


        for (const auto& pair : savedSchematics) {
            unique_names[pair.first] = true;
        }

        if (!currentSchematic.netlistCommands.empty() || savedSchematics.count(currentSchematic.name)) {
            unique_names[currentSchematic.name] = true;
        } else if (currentSchematic.name.rfind("draft", 0) == 0 && currentSchematic.netlistCommands.empty() && !savedSchematics.count(currentSchematic.name)) {

            unique_names[currentSchematic.name] = true;
        }

        std::vector<std::string> ordered_names_vec;
        for (const auto& pair : unique_names) {
            ordered_names_vec.push_back(pair.first);
        }
        std::sort(ordered_names_vec.begin(), ordered_names_vec.end());
        return ordered_names_vec;
    }
public:
    Schematic currentSchematic;
    std::map<std::string, std::vector<std::string>> savedSchematics;
    int schematicCounter = 1;
    CircuitSimulator() : currentSchematic("draft" + std::to_string(schematicCounter++)) {}
    bool parseSinParams(const std::string& val_str, double& offset, double& amp, double& freq, double& phase) {
        if (toUpper(val_str).rfind("SIN(", 0) == 0 && val_str.back() == ')') {
            std::string content = val_str.substr(4, val_str.length() - 5);
            std::replace(content.begin(), content.end(), ',', ' ');
            std::stringstream ss_params(content);
            if (ss_params >> offset >> amp >> freq) {
                if (!(ss_params >> phase)) {
                    phase = 0.0;
                }

                std::string remaining;
                if (ss_params >> remaining && !trim(remaining).empty()){

                    return false;
                }
                return true;
            }
        }
        return false;
    }
    void processCommand(const std::string& line) {
        std::stringstream ss(line);
        std::string command;
        ss >> command;

        try {
            if (command.empty()) return;
            std::string original_line = line;
            if (command == "-show") { /* ... (same as before) ... */
                std::string nextWord; ss >> nextWord;
                if (toLower(nextWord) == "existing") { ss >> nextWord; showExistingSchematics(); }
                else { std::cout << "Syntax error in command" << std::endl; }
            }
            else if (command == "-choose") { /* ... (same as before) ... */
                std::string nextWord, choiceStr; ss >> choiceStr; ss >> nextWord; ss >> nextWord;
                try { int choice = std::stoi(choiceStr); chooseSchematic(choice); }
                catch (const std::invalid_argument&) { std::cout << "-Error: Inappropriate input" << std::endl; }
                catch (const std::out_of_range&) { std::cout << "-Error: Inappropriate input (number too large)" << std::endl; }
            }
            else if (command == ".tran") {
                double t_end, step;
                if (!(ss >> t_end >> step)) {
                    std::cout << "Error: Invalid .tran syntax. Usage: .tran <t_end> <time_step>" << std::endl; return;
                }
                currentSchematic.simulateTransient(t_end, step);
            }
            else if (command == ".dc") {
                std::string sourceNameStr;
                double startValDouble, endValDouble, incrementDouble;

                if (!(ss >> sourceNameStr >> startValDouble >> endValDouble >> incrementDouble)) {
                    std::cout << "Error: Invalid .dc syntax. Usage: .DC <SourceName> <StartValue> <EndValue> <Increment>" << std::endl;
                } else {

                    if (!currentSchematic.groundNode) {
                        std::cout << "Error: No ground node defined in the circuit. DC analysis requires a ground." << std::endl;
                    } else {
                        currentSchematic.performDCSweep(sourceNameStr, startValDouble, endValDouble, incrementDouble);
                    }
                }
            }
            else if (toLower(command) == "newfile") {
                std::string filePath; ss >> filePath;
                if (filePath.empty()) { std::cout << "Error: File path missing for NewFile." << std::endl; }
                else { loadSchematicFromFile(filePath, true); }
            }
            else if (toLower(command) == "return") {
                std::cout << "Returning to main prompt (if applicable)." << std::endl;
            }
            else if (toLower(command) == "add") {
                std::string elementType, elementName, n1_name, n2_name;
                ss >> elementType;

                if (toLower(elementType) == "gnd") {
                    ss >> n1_name;
                    if (n1_name.empty()) { std::cout << "Error: Syntax error. Node name missing for add GND command." << std::endl; return; }
                    currentSchematic.addGroundCommand(n1_name, original_line);
                    std::cout << "Node " << n1_name << " set as ground." << std::endl;
                } else {
                    std::string val_full_str;
                    if (!(ss >> elementName >> n1_name >> n2_name)) {
                        std::cout << "Error: Syntax error. Incomplete add element command." << std::endl; return;
                    }
                    std::getline(ss, val_full_str);
                    val_full_str = trim(val_full_str);

                    if (val_full_str.empty()){
                        std::cout << "Error: Missing value/parameters for element " << elementName << std::endl; return;
                    }

                    char typePrefix = std::toupper(static_cast<unsigned char>(elementType[0]));
                    if (elementType.length() != 1 || !std::isalpha(static_cast<unsigned char>(typePrefix))) {
                        std::cout << "Error: Invalid element type '" << elementType << "'. Must be a single character (R, C, L, V, I)." << std::endl; return;
                    }

                    Node* n1 = currentSchematic.findOrCreateNode(n1_name);
                    Node* n2 = currentSchematic.findOrCreateNode(n2_name);
                    CircuitElement* newEl = nullptr;

                    double sin_offset, sin_amp, sin_freq, sin_phase;

                    try {
                        if (typePrefix == 'V') {
                            if (parseSinParams(val_full_str, sin_offset, sin_amp, sin_freq, sin_phase)) {
                                newEl = new VoltageSource(elementName, n1, n2, sin_offset, sin_amp, sin_freq, sin_phase);
                            } else {
                                newEl = new VoltageSource(elementName, n1, n2, parseValue(val_full_str));
                            }
                        } else if (typePrefix == 'I') {
                            if (parseSinParams(val_full_str, sin_offset, sin_amp, sin_freq, sin_phase)) {
                                newEl = new CurrentSource(elementName, n1, n2, sin_offset, sin_amp, sin_freq, sin_phase);
                            } else {
                                newEl = new CurrentSource(elementName, n1, n2, parseValue(val_full_str));
                            }
                        } else if (typePrefix == 'R') {
                            newEl = new Resistor(elementName, n1, n2, parseValue(val_full_str));
                        } else if (typePrefix == 'C') {
                            newEl = new Capacitor(elementName, n1, n2, parseValue(val_full_str));
                        } else if (typePrefix == 'L') {
                            newEl = new Inductor(elementName, n1, n2, parseValue(val_full_str));
                        }
                        else if (typePrefix == 'D') { // Diode
                            std::string diode_param_type_str; // "d" or "z"
                            std::stringstream val_ss(val_full_str);
                            val_ss >> diode_param_type_str;
                            diode_param_type_str = toLower(diode_param_type_str);

                            if (diode_param_type_str == "d") {
                                newEl = new Diode(elementName, n1, n2, "d");
                            } else if (diode_param_type_str == "z") {
                                double vz_val;
                                if (!(val_ss >> vz_val)) {
                                    throw std::runtime_error("Missing Zener voltage (Vz) for Zener diode " + elementName +
                                                             ". Expected format: add D <name> <n1> <n2> z <Vz_value>");
                                }
                                newEl = new Diode(elementName, n1, n2, "z", vz_val);
                            } else {
                                throw std::runtime_error("Invalid diode type parameter '" + diode_param_type_str +
                                                         "' for " + elementName + ". Must be 'd' (ideal) or 'z' (Zener).");
                            }
                        }
                        else {
                            std::cout << "Error: Unknown element type '" << elementType << "'" << std::endl; return;
                        }
                        currentSchematic.addElement(newEl, original_line);
                        std::cout << "Added: " << elementType << " " << elementName << " " << n1_name << " " << n2_name << " " << val_full_str << std::endl;

                    } catch (const std::exception& e) {
                        std::cout << "Error processing add command for " << elementName << " (" << val_full_str << "): " << e.what() << std::endl;
                        if(newEl) delete newEl; // Clean up if error during/after parsing but before adding
                        return;
                    }
                }
            }
            else if (toLower(command) == ".save") { // New command to save to file
                std::string filepath_to_save;
                if (!(ss >> filepath_to_save) || filepath_to_save.empty()) {
                    std::cout << "Error: Filepath missing for .save command. Usage: .save <filepath>" << std::endl;
                } else {
                    if (currentSchematic.netlistCommands.empty()) {
                        std::cout << "Warning: Current schematic is empty. An empty file will be created/overwritten." << std::endl;
                    }
                    saveSchematicToFile(filepath_to_save);
                }
            }
            else if (toLower(command) == "delete") { /* ... (same as before) ... */
                std::string firstArg, secondArg; ss >> firstArg;
                if (!ss.eof() && !(ss >> secondArg).fail()){ currentSchematic.deleteElementCommand(firstArg + " " + secondArg); }
                else { currentSchematic.deleteElementCommand(firstArg); }
            }
            else if (command == ".nodes") { currentSchematic.listNodes(); }
            else if (command == ".list") {
                std::string typeFilter; ss >> typeFilter; currentSchematic.listElements(typeFilter); }
            else if (command == ".all"){
                currentSchematic.listNodes();
                std::string typeFilter; ss >> typeFilter; currentSchematic.listElements(typeFilter);
            }
            else if (command == ".rename") {
                std::string targetType, oldName, newName;
                if (!(ss >> targetType >> oldName >> newName)) { // Ensure all three are read
                    std::cout << "ERROR: Invalid syntax. Usage: .rename <node|element> <old_name> <new_name>" << std::endl;
                    return;
                }

                if (toLower(targetType) == "node") {
                    currentSchematic.renameNode(oldName, newName);
                } else if (toLower(targetType) == "element") {
                    currentSchematic.renameElement(oldName, newName);
                } else {
                    std::cout << "ERROR: Invalid target type for .rename. Must be 'node' or 'element'." << std::endl;
                }
            }
            else if (command.length() > 0 && std::isalpha(static_cast<unsigned char>(command[0]))) {
                std::string elNameFromFile = command;
                char typePrefixFromFile = std::toupper(static_cast<unsigned char>(elNameFromFile[0]));
                std::string n1_name, n2_name;
                std::string val_full_str_file; // To capture value/SIN params

                if (!(ss >> n1_name >> n2_name)) {
                    std::cout << "Unknown command or incomplete element definition (missing nodes): " << original_line << std::endl; return;
                }
                std::getline(ss, val_full_str_file);
                val_full_str_file = trim(val_full_str_file);

                if (val_full_str_file.empty()){
                    std::cout << "Unknown command or incomplete element definition (missing value/params): " << original_line << std::endl; return;
                }

                Node* n1 = currentSchematic.findOrCreateNode(n1_name);
                Node* n2 = currentSchematic.findOrCreateNode(n2_name);
                CircuitElement* newElFile = nullptr;
                double sin_offset, sin_amp, sin_freq, sin_phase;

                try {
                    if (typePrefixFromFile == 'V') {
                        if (parseSinParams(val_full_str_file, sin_offset, sin_amp, sin_freq, sin_phase)) {
                            newElFile = new VoltageSource(elNameFromFile, n1, n2, sin_offset, sin_amp, sin_freq, sin_phase);
                        } else {
                            newElFile = new VoltageSource(elNameFromFile, n1, n2, parseValue(val_full_str_file));
                        }
                    } else if (typePrefixFromFile == 'I') {
                        if (parseSinParams(val_full_str_file, sin_offset, sin_amp, sin_freq, sin_phase)) {
                            newElFile = new CurrentSource(elNameFromFile, n1, n2, sin_offset, sin_amp, sin_freq, sin_phase);
                        } else {
                            newElFile = new CurrentSource(elNameFromFile, n1, n2, parseValue(val_full_str_file));
                        }
                    } else if (typePrefixFromFile == 'R') {
                        newElFile = new Resistor(elNameFromFile, n1, n2, parseValue(val_full_str_file));
                    } else if (typePrefixFromFile == 'C') {
                        newElFile = new Capacitor(elNameFromFile, n1, n2, parseValue(val_full_str_file));
                    } else if (typePrefixFromFile == 'L') {
                        newElFile = new Inductor(elNameFromFile, n1, n2, parseValue(val_full_str_file));
                    }
                    else if (typePrefixFromFile == 'D') {
                        std::string diode_param_type_str_file;
                        std::stringstream val_ss_file(val_full_str_file);
                        val_ss_file >> diode_param_type_str_file;
                        diode_param_type_str_file = toLower(diode_param_type_str_file);

                        if (diode_param_type_str_file == "d") {
                            newElFile = new Diode(elNameFromFile, n1, n2, "d");
                        } else if (diode_param_type_str_file == "z") {
                            double vz_val_file;
                            if (!(val_ss_file >> vz_val_file)) {
                                throw std::runtime_error("Missing Zener voltage (Vz) for Zener diode from file: " + elNameFromFile +
                                                         ". Expected format: <D_name> <n1> <n2> z <Vz_value>");
                            }
                            newElFile = new Diode(elNameFromFile, n1, n2, "z", vz_val_file);
                        } else {
                            throw std::runtime_error("Invalid diode type parameter '" + diode_param_type_str_file +
                                                     "' from file for " + elNameFromFile + ". Must be 'd' or 'z'.");
                        }
                    }
                    else {
                        std::cout << "Warning: Parsed line as potential element, but type '" << typePrefixFromFile << "' is not standard or line is misformatted: " << original_line << std::endl;
                        return;
                    }
                    currentSchematic.addElement(newElFile, original_line);
                    // No "Added:" message here, assuming file load is less verbose or handled by loadSchematicFromFile
                    if (toLower(n1_name) == "gnd" || n1_name == "0") currentSchematic.addGroundCommand(n1_name, "add GND " + n1_name);
                    if (toLower(n2_name) == "gnd" || n2_name == "0") currentSchematic.addGroundCommand(n2_name, "add GND " + n2_name);

                } catch (const std::exception& e) {
                    std::cout << "Error parsing element from file style for " << elNameFromFile << " (" << val_full_str_file << "): " << e.what() << std::endl;
                    if(newElFile) delete newElFile;
                    return;
                }
            }
            else { std::cout << "Unknown command: " << original_line << std::endl; }
        }
        catch (const std::exception& e) { std::cout << "Error: " << e.what() << std::endl; }
    }
    void saveCurrentSchematic(const std::string& name) { /* ... (same as before) ... */
        if (name.empty()) { std::cout << "Error: Schematic name cannot be empty for saving." << std::endl; return; }
        currentSchematic.name = name; savedSchematics[name] = currentSchematic.netlistCommands;
        std::cout << "Schematic '" << name << "' saved (in memory)." << std::endl;
    }
    void showExistingSchematics() {
        std::vector<std::string> ordered_names = _getAvailableSchematicNamesForMenu();

        if (ordered_names.empty()) {
            std::cout << "No schematics available to choose." << std::endl;
            return;
        }

        std::cout << "-choose existing schematic:" << std::endl;
        int i = 1;
        for (const auto& name_to_show : ordered_names) {
            std::string suffix = "";
            if (name_to_show == currentSchematic.name) {
                // Check if the current schematic is identical to a saved version under its name
                bool is_identical_to_saved = savedSchematics.count(name_to_show) &&
                                             savedSchematics.at(name_to_show) == currentSchematic.netlistCommands;
                if (is_identical_to_saved) {
                    suffix = " (current)";
                } else {
                    // If it's the current one but not identical to a saved version, or not saved at all under this name
                    suffix = " (current, unsaved/modified)";
                }
            }
            std::cout << i++ << "-" << name_to_show << suffix << std::endl;
        }
    }
    void chooseSchematic(int choice) {
        if (choice <= 0) {
            std::cout << "-Error: Inappropriate input" << std::endl;
            return;
        }

        std::vector<std::string> available_choices = _getAvailableSchematicNamesForMenu();

        if (choice > 0 && static_cast<size_t>(choice) <= available_choices.size()) {
            std::string chosenName = available_choices[choice - 1];

            if (currentSchematic.name != chosenName) { // Only consider saving if switching to a DIFFERENT schematic
                bool isCurrentSavedInMemory = savedSchematics.count(currentSchematic.name) &&
                                              savedSchematics.at(currentSchematic.name) == currentSchematic.netlistCommands;

                // Use !currentSchematic.netlistCommands.empty() for content check
                if (!isCurrentSavedInMemory && !currentSchematic.netlistCommands.empty()) {
                    saveCurrentSchematic(currentSchematic.name);
                    // Optional: Add a message like "Saved changes to '...' before switching."
                }
            }

            // Proceed to load/switch
            if (savedSchematics.count(chosenName)) {
                // Check if we are "choosing" the current schematic that's already loaded and identical
                if (chosenName == currentSchematic.name &&
                    savedSchematics.at(chosenName) == currentSchematic.netlistCommands) {
                    std::cout << "Schematic '" << chosenName << "' is already current." << std::endl;
                    return;
                }

                currentSchematic.clear();
                currentSchematic.name = chosenName; // Set name before processing commands
                std::vector<std::string> cmds_to_load = savedSchematics[chosenName];
                for (const std::string& cmd : cmds_to_load) {
                    processCommand(cmd); // Reprocess
                }
                std::cout << "Loaded schematic: " << chosenName << std::endl;
            } else if (chosenName == currentSchematic.name) {
                // This case means chosenName is currentSchematic.name, but it's not in savedSchematics
                // (e.g., an unsaved draft), or it is in savedSchematics but current is modified.
                // Since we didn't switch names, no further action other than confirming.
                std::cout << "Keeping current schematic: " << currentSchematic.name << std::endl;
            } else {
                // This branch should ideally not be hit if _getAvailableSchematicNamesForMenu is correct,
                // as all names from the menu should either be in savedSchematics or be currentSchematic.name.
                // However, if it implies switching to a new (e.g., empty draft listed by name):
                currentSchematic.clear();
                currentSchematic.name = chosenName;
                std::cout << "Switched to (new/empty) schematic: " << chosenName << std::endl;

            }
        } else {
            std::cout << "-Error: Inappropriate input. Choice " << choice << " is out of range." << std::endl;
        }
    }
    void loadSchematicFromFile(const std::string& filepath, bool clearCurrent = true) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "Error: Could not open file '" << filepath << "'. Current schematic remains unchanged." << std::endl;
            return; // Exit early, currentSchematic is not affected
        }
        // --- End Correction ---

        if (clearCurrent) {
            // Check if the current in-memory schematic needs saving before being overwritten
            bool isCurrentSavedInMemory = savedSchematics.count(currentSchematic.name) &&
                                          savedSchematics.at(currentSchematic.name) == currentSchematic.netlistCommands;

            if (!isCurrentSavedInMemory && !currentSchematic.netlistCommands.empty()) {
                saveCurrentSchematic(currentSchematic.name); // Saves to in-memory map
                std::cout << "Saved changes to '" << currentSchematic.name << "' before loading new file." << std::endl;
            }

            currentSchematic.clear(); // Clears netlistCommands, elements, nodes, etc.

            // Set the name of the currentSchematic to the name of the file being loaded
            size_t lastSlash = filepath.find_last_of("/\\");
            currentSchematic.name = (lastSlash == std::string::npos) ? filepath : filepath.substr(lastSlash + 1);
            size_t lastDot = currentSchematic.name.find_last_of('.');
            if (lastDot != std::string::npos) {
                currentSchematic.name = currentSchematic.name.substr(0, lastDot);
            }
        }
        // Note: The 'else' part of 'if (clearCurrent)' (i.e., appending to current schematic)
        // is not currently implemented or directly supported by 'newfile'.
        // 'newfile' always intends to replace the current schematic.

        std::cout << "Loading schematic from file: " << filepath << std::endl;
        std::vector<std::string> fileCommands;
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '*' || line[0] == ';') continue; // Skip comments and empty lines
            if (toLower(line) == ".end") break; // Standard SPICE end directive
            fileCommands.push_back(line);
        }
        file.close();

        // Process the commands read from the file
        for (const std::string& cmd_from_file : fileCommands) {
            processCommand(cmd_from_file); // This will add elements to the (now empty) currentSchematic
        }

        std::cout << "Finished loading schematic '" << currentSchematic.name << "' from " << filepath << std::endl;
        saveCurrentSchematic(currentSchematic.name);
    }
    void saveSchematicToFile(const std::string& filepath) {
        if (filepath.empty()) {
            std::cout << "Error: Filepath cannot be empty for saving." << std::endl;
            return;
        }

        std::ofstream outFile(filepath);
        if (!outFile.is_open()) {
            std::cout << "Error: Could not open file '" << filepath << "' for writing." << std::endl;
            return;
        }

        std::cout << "Saving schematic to file: " << filepath << std::endl;

        // Iterate through the stored netlist commands and write them in a parseable format
        for (const auto& cmd : currentSchematic.netlistCommands) {
            std::string original_command_line = cmd;
            std::stringstream ss_parser(original_command_line);
            std::string first_token;
            ss_parser >> first_token;

            std::string line_to_write_to_file;

            if (toLower(first_token) == "add") {
                std::string el_type_prefix_char, el_name;
                ss_parser >> el_type_prefix_char; // e.g., "R", "V", "D", "GND"

                if (toLower(el_type_prefix_char) == "gnd") {
                    // GND definitions are implicit by components connecting to node "0" or "gnd".
                    // Explicit "add GND" commands are for internal state and not typically written to SPICE files.
                    continue;
                }

                ss_parser >> el_name; // e.g., "R1", "Vsource", "D1"

                // The rest of the stream contains nodes and value/parameters
                std::string rest_of_params;
                std::getline(ss_parser, rest_of_params); // Read the remainder of the line
                rest_of_params = trim(rest_of_params);   // Trim whitespace

                line_to_write_to_file = el_name + " " + rest_of_params;

            } else if (!first_token.empty() && first_token[0] != '.' &&
                       std::isalpha(static_cast<unsigned char>(first_token[0]))) {
                // This looks like a direct SPICE-style line already, e.g., "R1 N1 N2 100"
                // It was stored directly from user input or a file.
                line_to_write_to_file = original_command_line;
            } else {
                // This is likely a '.' command (like .rename, .nodes, .list, .tran)
                // or an empty/invalid line. These are generally not part of the
                // pure netlist definition saved to a file (simulation commands like .tran are sometimes included).
                // For now, we'll skip them to keep the file focused on component definitions.
                // You can choose to include specific .commands like .tran if desired.
                continue;
            }

            if (!line_to_write_to_file.empty()) {
                outFile << line_to_write_to_file << std::endl;
            }
        }

        // Optional: Add a .END statement, common in SPICE files
        // outFile << ".END" << std::endl;

        outFile.close();
        if (outFile.fail()) { // Check if close failed (e.g., disk full during flush)
            std::cout << "Error: Failed to properly close the file after writing: " << filepath << std::endl;
        } else {
            std::cout << "Schematic successfully saved to file: " << filepath << std::endl;

            // Optional: Update currentSchematic.name and save to in-memory map
            // This makes the behavior consistent with loadSchematicFromFile
            size_t lastSlash = filepath.find_last_of("/\\");
            std::string newName = (lastSlash == std::string::npos) ? filepath : filepath.substr(lastSlash + 1);
            size_t lastDot = newName.find_last_of('.');
            if (lastDot != std::string::npos) {
                newName = newName.substr(0, lastDot);
            }
            // Only update name if it's different, or if current name is a draft
            if (currentSchematic.name != newName || currentSchematic.name.rfind("draft",0)==0) {
                currentSchematic.name = newName;
            }
            // This ensures that after a file save, the in-memory representation also reflects this "saved" state
            // under the new (file-derived) name.
            saveCurrentSchematic(currentSchematic.name);
        }
    }
    void run() {
        std::string line;
        while (true) {
            std::cout << "> "; // Prompt
            if (!std::getline(std::cin, line)) break;
            line = trim(line);
            if (toLower(line) == "exit" || toLower(line) == "quit") {
                // Use !currentSchematic.netlistCommands.empty() for content check
                if (!currentSchematic.netlistCommands.empty()){
                    bool isCurrentSavedInMemory = savedSchematics.count(currentSchematic.name) > 0 &&
                                                  savedSchematics.at(currentSchematic.name) == currentSchematic.netlistCommands;
                    // Only prompt for named schematics or if it's a draft that the user explicitly saved with a name earlier
                    if (!isCurrentSavedInMemory && currentSchematic.name.rfind("draft",0) != 0){
                        std::cout << "Schematic '" << currentSchematic.name << "' has unsaved changes. Save it? (yes/no/new_name): ";
                        std::string saveChoice; std::getline(std::cin, saveChoice); saveChoice = trim(toLower(saveChoice));
                        if(saveChoice == "yes") saveCurrentSchematic(currentSchematic.name);
                        else if (saveChoice != "no" && !saveChoice.empty()) saveCurrentSchematic(saveChoice);
                    } else if (!isCurrentSavedInMemory && savedSchematics.count(currentSchematic.name)) {
                        // This case covers a "draftX" that was previously saved (e.g. by switching away from it)
                        // and then modified again.
                        std::cout << "Schematic '" << currentSchematic.name << "' has unsaved changes. Save it? (yes/no/new_name): ";
                        std::string saveChoice; std::getline(std::cin, saveChoice); saveChoice = trim(toLower(saveChoice));
                        if(saveChoice == "yes") saveCurrentSchematic(currentSchematic.name);
                        else if (saveChoice != "no" && !saveChoice.empty()) saveCurrentSchematic(saveChoice);
                    }
                }
                break;
            }
            if (line.empty()) continue;
            processCommand(line);
        }
        std::cout << "Exiting simulator." << std::endl;
    }

};
int main() {
    CircuitSimulator sim;
    sim.run();
    return 0;
}
