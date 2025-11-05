
class Model {
   public:
    std::vector<Field> fields;

    Field* getById(const String& id) {
        for (auto& f : fields)
            if (f.getId() == id) return &f;
        return nullptr;
    }

    Field* getByName(const String& name) {
        for (auto& f : fields)
            if (f.getName() == name) return &f;
        return nullptr;
    }

    void add(Field f) { fields.push_back(f); }

    bool remove(const String& id) {
        for (size_t i = 0; i < fields.size(); i++) {
            if (fields[i].getId() == id) {
                fields.erase(fields.begin() + i);
                return true;
            }
        }
        return false;
    }

    void reorder(const String& id, bool up) {
        for (size_t i = 0; i < fields.size(); i++) {
            if (fields[i].getId() == id) {
                if (up && i > 0)
                    std::swap(fields[i], fields[i - 1]);
                else if (!up && i < fields.size() - 1)
                    std::swap(fields[i], fields[i + 1]);
                break;
            }
        }
    }

    void initialize() {
        Serial.println("[MODEL] Loading factory model");
        Helper::initialize(model.fields);
    }

    bool load() {
        JsonWrapper::loadFieldsFromFile(fields);
        if (fields.empty()) {
            initialize();
            return false;
        }
        return true;
    }

    bool saveToFile() {
        File file = SPIFFS.open("/model.json", "w");
        if (!file) {
            Serial.println("error in model saveToFile file");
            return false;
        }
        return JsonWrapper::saveModelToFile(fields);
    }

    String fieldsToJsonString() {
        return JsonWrapper::fieldsToJsonString(fields);
    }

    void listSerial() {
        for (auto& f : fields) Serial.printf("%s (%s) = %s\n", f.getName().c_str(), f.getType().c_str(), f.getValue().c_str());
    }
};  // end Model
