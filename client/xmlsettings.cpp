#include "PreCompile.h"
#include "xmlsettings.h"

XMLSettings::XMLSettings()
  : m_IsReadOnly(false)
  , m_IsOpen(false)
  , m_HasChanges(false)
  , m_RootNode(nullptr)
{
  Open("config.xml");
}

XMLSettings::XMLSettings(const QString& filename)
{
  Open(filename);
}

XMLSettings::~XMLSettings()
{

}

bool XMLSettings::IsReadOnly() const
{
  return false;
}

bool XMLSettings::IsOpen() const
{
  return false;
}

bool XMLSettings::HasChanges() const
{
  return false;
}

QString XMLSettings::GetValue(const QString& path, const QString& default) const
{
  return default;
}
void XMLSettings::SetValue(const QString& path, const QString& value)
{

}

bool XMLSettings::Save()
{
  return false;
}
bool XMLSettings::Reload()
{
  return false;
}

bool XMLSettings::Open(const QString& filepath)
{
  m_IsOpen = false;
  m_IsReadOnly = false;
  m_HasChanges = false;

  QFile config(filepath);
  if (!config.open(QIODevice::ReadWrite))
  {
    if (!config.open(QIODevice::ReadOnly))
    {
      return false;
    }
    m_IsReadOnly = true;
  }
  m_IsOpen = true;

  QDomDocument doc;
  if (!doc.setContent(&config))
  {
    m_IsOpen = false;
    m_IsReadOnly = false;
    return false;
  }

  m_RootNode = new ConfigNode(doc.documentElement().tagName());
  //for (auto child : doc.documentElement().childNodes())
  //{ }


  return true;
}
