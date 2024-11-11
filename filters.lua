local meta = {}
local title = nil
local stringify = pandoc.utils.stringify

function get_title(h)
  if h.level == 1 and not title then
    title = h.content
    return h
  end
end

function change_meta(m)
  meta = m
  if m.page == "index" then
    m.canonical = stringify(m.baseurl) .. "/"
  else
    m.canonical = stringify(m.baseurl) .. "/" .. m.page .. "/"
  end
  m.title = title or m.site_title
  return m
end

function change_link_target(l)
  if meta[l.target] then
    l.target = stringify(meta[l.target])
  elseif l.target:sub(1, 1) == "/" then
    l.target = meta.base .. l.target
  end
  return l
end

function change_image_src(i)
  if i.src:sub(1, 1) == "/" then
    i.src = meta.base .. i.src
  end
  return i
end

return {
  {
    Header = get_title,
    Meta = change_meta
  },
  {
    Link = change_link_target,
    Image = change_image_src
  }
}
