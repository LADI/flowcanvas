/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "raul/log.hpp"
#include "raul/PathTable.hpp"
#include "interface/MessageType.hpp"
#include "ClientStore.hpp"
#include "ObjectModel.hpp"
#include "PatchModel.hpp"
#include "NodeModel.hpp"
#include "PortModel.hpp"
#include "PluginModel.hpp"
#include "PatchModel.hpp"
#include "SigClientInterface.hpp"

#define LOG(s) s << "[ClientStore] "

using namespace std;
using namespace Raul;

namespace Ingen {
using namespace Shared;
namespace Client {


ClientStore::ClientStore(SharedPtr<EngineInterface> engine, SharedPtr<SigClientInterface> emitter)
	: _engine(engine)
	, _emitter(emitter)
	, _plugins(new Plugins())
{
	if (!emitter)
		return;

	emitter->signal_object_deleted.connect(sigc::mem_fun(this, &ClientStore::del));
	emitter->signal_object_moved.connect(sigc::mem_fun(this, &ClientStore::move));
	emitter->signal_put.connect(sigc::mem_fun(this, &ClientStore::put));
	emitter->signal_connection.connect(sigc::mem_fun(this, &ClientStore::connect));
	emitter->signal_disconnection.connect(sigc::mem_fun(this, &ClientStore::disconnect));
	emitter->signal_property_change.connect(sigc::mem_fun(this, &ClientStore::set_property));
	emitter->signal_port_value.connect(sigc::mem_fun(this, &ClientStore::set_port_value));
	emitter->signal_voice_value.connect(sigc::mem_fun(this, &ClientStore::set_voice_value));
	emitter->signal_activity.connect(sigc::mem_fun(this, &ClientStore::activity));
	emitter->signal_binding.connect(sigc::mem_fun(this, &ClientStore::binding));
}


void
ClientStore::clear()
{
	Store::clear();
	_plugins->clear();
}


void
ClientStore::add_object(SharedPtr<ObjectModel> object)
{
	// If we already have "this" object, merge the existing one into the new
	// one (with precedence to the new values).
	iterator existing = find(object->path());
	if (existing != end()) {
		PtrCast<ObjectModel>(existing->second)->set(object);
	} else {
		if (!object->path().is_root()) {
			SharedPtr<ObjectModel> parent = this->object(object->path().parent());
			if (parent) {
				assert(object->path().is_child_of(parent->path()));
				object->set_parent(parent);
				parent->add_child(object);
				assert(parent && (object->parent() == parent));

				(*this)[object->path()] = object;
				signal_new_object.emit(object);
			}
		} else {
			(*this)[object->path()] = object;
			signal_new_object.emit(object);
		}

	}

	for (Resource::Properties::const_iterator i = object->meta().properties().begin();
			i != object->meta().properties().end(); ++i)
		object->signal_property(i->first, i->second);

	for (Resource::Properties::const_iterator i = object->properties().begin();
			i != object->properties().end(); ++i)
		object->signal_property(i->first, i->second);

	LOG(debug) << "Added " << object->path() << " {" << endl;
	for (iterator i = begin(); i != end(); ++i) {
		LOG(debug) << "\t" << i->first << endl;
	}
	LOG(debug) << "}" << endl;
}


SharedPtr<ObjectModel>
ClientStore::remove_object(const Path& path)
{
	iterator i = find(path);

	if (i != end()) {
		assert((*i).second->path() == path);
		SharedPtr<ObjectModel> result = PtrCast<ObjectModel>((*i).second);
		assert(result);
		iterator descendants_end = find_descendants_end(i);
		SharedPtr<Store::Objects> removed = yank(i, descendants_end);

		LOG(debug) << "[ClientStore] Removing " << i->first << " {" << endl;
		for (iterator i = removed->begin(); i != removed->end(); ++i) {
			LOG(debug) << "\t" << i->first << endl;
		}
		LOG(debug) << "}" << endl;

		if (result)
			result->signal_destroyed.emit();

		if (!result->path().is_root()) {
			assert(result->parent());

			SharedPtr<ObjectModel> parent = this->object(result->path().parent());
			if (parent) {
				parent->remove_child(result);
			}
		}

		assert(!object(path));

		return result;

	} else {
		return SharedPtr<ObjectModel>();
	}
}


SharedPtr<PluginModel>
ClientStore::plugin(const URI& uri)
{
	assert(uri.length() > 0);
	Plugins::iterator i = _plugins->find(uri);
	if (i == _plugins->end())
		return SharedPtr<PluginModel>();
	else
		return (*i).second;
}


SharedPtr<ObjectModel>
ClientStore::object(const Path& path)
{
	assert(path.length() > 0);
	iterator i = find(path);
	if (i == end()) {
		return SharedPtr<ObjectModel>();
	} else {
		SharedPtr<ObjectModel> model = PtrCast<ObjectModel>(i->second);
		assert(model);
		assert(model->path().is_root() || model->parent());
		return model;
	}
}

SharedPtr<Resource>
ClientStore::resource(const URI& uri)
{
	if (uri.scheme() == Path::scheme && Path::is_valid(uri.str()))
		return object(uri.str());
	else
		return plugin(uri);
}

void
ClientStore::add_plugin(SharedPtr<PluginModel> pm)
{
	SharedPtr<PluginModel> existing = this->plugin(pm->uri());
	if (existing) {
		existing->set(pm);
	} else {
		_plugins->insert(make_pair(pm->uri(), pm));
		signal_new_plugin(pm);
	}
}


/* ****** Signal Handlers ******** */


void
ClientStore::del(const Path& path)
{
	SharedPtr<ObjectModel> removed = remove_object(path);
	removed.reset();
	debug << "[ClientStore] removed object " << path << ", count: " << removed.use_count();
}

void
ClientStore::move(const Path& old_path_str, const Path& new_path_str)
{
	Path old_path(old_path_str);
	Path new_path(new_path_str);

	iterator parent = find(old_path);
	if (parent == end()) {
		LOG(error) << "Failed to find object " << old_path << " to move." << endl;
		return;
	}

	iterator descendants_end = find_descendants_end(parent);

	SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > > removed
			= yank(parent, descendants_end);

	assert(removed->size() > 0);

	for (Table<Path, SharedPtr<Shared::GraphObject> >::iterator i = removed->begin(); i != removed->end(); ++i) {
		const Path& child_old_path = i->first;
		assert(Path::descendant_comparator(old_path, child_old_path));

		Path child_new_path;
		if (child_old_path == old_path)
			child_new_path = new_path;
		else
			child_new_path = new_path.base() + child_old_path.substr(old_path.length()+1);

		LOG(info) << "Renamed " << child_old_path << " -> " << child_new_path << endl;
		PtrCast<ObjectModel>(i->second)->set_path(child_new_path);
		i->first = child_new_path;
	}

	cram(*removed.get());
}


void
ClientStore::put(const URI& uri, const Resource::Properties& properties)
{
	typedef Resource::Properties::const_iterator iterator;
	LOG(debug) << "PUT " << uri << " {" << endl;
	for (iterator i = properties.begin(); i != properties.end(); ++i)
		LOG(debug) << "    " << i->first << " = " << i->second << " :: " << i->second.type() << endl;
	LOG(debug) << "}" << endl;

	bool is_path = Path::is_valid(uri.str());
	bool is_meta = ResourceImpl::is_meta_uri(uri);

	if (!(is_path || is_meta)) {
		const URI& type_uri = properties.find("rdf:type")->second.get_uri();
		if (Plugin::type_from_uri(type_uri.str()) != Plugin::NIL) {
			SharedPtr<PluginModel> p(new PluginModel(uri, type_uri, properties));
			add_plugin(p);
			return;
		}
	}

	string path_str = is_meta ? (string("/") + uri.chop_start("#")) : uri.str();
	if (!Path::is_valid(path_str)) {
		LOG(error) << "Bad path: " << uri.str() << " - " << path_str << endl;
		return;
	}

	Path path(is_meta ? (string("/") + uri.chop_start("#")) : uri.str());

	SharedPtr<ObjectModel> obj = PtrCast<ObjectModel>(object(path));
	if (obj) {
		obj->set_properties(properties);
		return;
	}

	bool is_patch, is_node, is_port, is_output;
	PortType data_type(PortType::UNKNOWN);
	ResourceImpl::type(properties, is_patch, is_node, is_port, is_output, data_type);

	if (is_patch) {
		uint32_t poly = 1;
		iterator p = properties.find("ingen:polyphony");
		if (p != properties.end() && p->second.is_valid() && p->second.type() == Atom::INT)
			poly = p->second.get_int32();
		SharedPtr<PatchModel> model(new PatchModel(path, poly));
		model->set_properties(properties);
		add_object(model);
	} else if (is_node) {
		const Resource::Properties::const_iterator p = properties.find("rdf:instanceOf");
		SharedPtr<PluginModel> plug;
		if (p->second.is_valid() && p->second.type() == Atom::URI) {
			if (!(plug = plugin(p->second.get_uri()))) {
				LOG(warn) << "Unable to find plugin " << p->second.get_uri() << endl;
				plug = SharedPtr<PluginModel>(
						new PluginModel(p->second.get_uri(), "ingen:nil", Resource::Properties()));
				add_plugin(plug);
			}

			SharedPtr<NodeModel> n(new NodeModel(plug, path));
			n->set_properties(properties);
			add_object(n);
		} else {
			LOG(error) << "Plugin with no type" << endl;
		}
	} else if (is_port) {
		if (data_type != PortType::UNKNOWN) {
			PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;
			SharedPtr<PortModel> p(new PortModel(path, 0, data_type, pdir));
			p->set_properties(properties);
			add_object(p);
		} else {
			LOG(warn) << "Port " << path << " has no type" << endl;
		}
	} else {
		LOG(warn) << "Ignoring object " << path << " with unknown type "
			<< is_patch << " " << is_node << " " << is_port << endl;
	}
}


void
ClientStore::set_property(const URI& subject_uri, const URI& predicate, const Atom& value)
{
	SharedPtr<Resource> subject = resource(subject_uri);

	size_t hash = subject_uri.find("#");
	if (!value.is_valid()) {
		LOG(error) << "Property '" << predicate << "' is invalid" << endl;
	} else if (subject) {
		subject->set_property(predicate, value);
	} else if (ResourceImpl::is_meta_uri(subject_uri)) {
		Path instance_path = string("/") + subject_uri.substr(hash + 1);
		SharedPtr<ObjectModel> om = PtrCast<ObjectModel>(subject);
		if (om)
			om->meta().set_property(predicate, value);
	} else {
		SharedPtr<PluginModel> plugin = this->plugin(subject_uri);
		if (plugin)
			plugin->set_property(predicate, value);
		else
			LOG(warn) << "Property '" << predicate << "' for unknown object " << subject_uri << endl;
	}
}


void
ClientStore::set_port_value(const Path& port_path, const Atom& value)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(value);
	else
		LOG(error) << "Control change for non-existent port " << port_path << endl;
}


void
ClientStore::set_voice_value(const Path& port_path, uint32_t voice, const Atom& value)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(port_path));
	if (port)
		port->value(voice, value);
	else
		LOG(error) << "Polyphonic control change for non-existent port " << port_path << endl;
}


void
ClientStore::activity(const Path& path)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(object(path));
	if (port)
		port->signal_activity.emit();
	else
		LOG(error) << "Activity for non-existent port " << path << endl;
}


void
ClientStore::binding(const Path& path, const Shared::MessageType& type)
{
	LOG(info) << "Bind " << path << " : " << type << endl;
}


SharedPtr<PatchModel>
ClientStore::connection_patch(const Path& src_port_path, const Path& dst_port_path)
{
	SharedPtr<PatchModel> patch;

	if (src_port_path.parent() == dst_port_path.parent())
		patch = PtrCast<PatchModel>(this->object(src_port_path.parent()));

	if (!patch && src_port_path.parent() == dst_port_path.parent().parent())
		patch = PtrCast<PatchModel>(this->object(src_port_path.parent()));

	if (!patch && src_port_path.parent().parent() == dst_port_path.parent())
		patch = PtrCast<PatchModel>(this->object(dst_port_path.parent()));

	if (!patch)
		patch = PtrCast<PatchModel>(this->object(src_port_path.parent().parent()));

	if (!patch)
		LOG(error) << "Unable to find connection patch " << src_port_path
			<< " -> " << dst_port_path << endl;

	return patch;
}


bool
ClientStore::attempt_connection(const Path& src_port_path, const Path& dst_port_path)
{
	SharedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	SharedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	if (src_port && dst_port) {
		assert(src_port->parent());
		assert(dst_port->parent());

		SharedPtr<PatchModel> patch = connection_patch(src_port_path, dst_port_path);
		assert(patch);

		SharedPtr<ConnectionModel> cm(new ConnectionModel(src_port, dst_port));

		src_port->connected_to(dst_port);
		dst_port->connected_to(src_port);

		patch->add_connection(cm);
		return true;
	}

	return false;
}


void
ClientStore::connect(const Path& src_port_path, const Path& dst_port_path)
{
	attempt_connection(src_port_path, dst_port_path);
}


void
ClientStore::disconnect(const Path& src_port_path, const Path& dst_port_path)
{
	// Find the ports and create a ConnectionModel just to get at the parent path
	// finding logic in ConnectionModel.  So I'm lazy.

	SharedPtr<PortModel> src_port = PtrCast<PortModel>(object(src_port_path));
	SharedPtr<PortModel> dst_port = PtrCast<PortModel>(object(dst_port_path));

	if (src_port)
		src_port->disconnected_from(dst_port);
	else
		LOG(warn) << "Disconnection from non-existent src port " << src_port_path << endl;

	if (dst_port)
		dst_port->disconnected_from(dst_port);
	else
		LOG(warn) << "Disconnection from non-existent dst port " << dst_port_path << endl;

	SharedPtr<PatchModel> patch = connection_patch(src_port_path, dst_port_path);

	if (patch)
		patch->remove_connection(src_port_path, dst_port_path);
	else
		LOG(error) << "Disconnection in non-existent patch: "
			<< src_port_path << " -> " << dst_port_path << endl;
}


} // namespace Client
} // namespace Ingen

