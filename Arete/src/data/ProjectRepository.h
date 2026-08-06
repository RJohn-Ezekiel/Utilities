#pragma once

#include "models/Project.h"
#include <QObject>
#include <QVector>

namespace arete::data {

class DatabaseManager;

class ProjectRepository : public QObject
{
    Q_OBJECT

public:
    explicit ProjectRepository(DatabaseManager* db, QObject* parent = nullptr);

    [[nodiscard]] QVector<models::Project> all(bool includeArchived = false) const;
    [[nodiscard]] models::Project byId(qint64 id) const;
    qint64 insert(const models::Project& project);
    bool update(const models::Project& project);
    bool remove(qint64 id);
    bool setArchived(qint64 id, bool archived);

signals:
    void changed();

private:
    DatabaseManager* m_db;
};

} // namespace arete::data
