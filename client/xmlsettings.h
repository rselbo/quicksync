#ifndef XMLSETTINGS_H
#define XMLSETTINGS_H
#pragma once

class XMLSettings
{
public:
  /// Opens the best config file it can
  XMLSettings();
  /// loads the specified config file
  XMLSettings(const QString& filename);
  /// Destroys the object without saving it
  ~XMLSettings();

  bool IsReadOnly() const;
  bool IsOpen() const;
  bool HasChanges() const;

  QString GetValue(const QString& path, const QString& default = EmptyQString) const;
  void    SetValue(const QString& path, const QString& value);

  bool Save();
  bool Reload();

private:
  bool Open(const QString& filepath);
  bool m_IsReadOnly;
  bool m_IsOpen;
  bool m_HasChanges;
  struct ConfigNode{
    ConfigNode(const QString& name) : m_Name(name), m_FirstChild(nullptr), m_NextSibling(nullptr) {}
    QString m_Name;
    ConfigNode* m_FirstChild;
    ConfigNode* m_NextSibling;
  }* m_RootNode;

  XMLSettings(const XMLSettings&);
  XMLSettings(const XMLSettings&&);
};

#endif //XMLSETTINGS_H