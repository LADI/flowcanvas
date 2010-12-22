/* JACK MIDI API compatibility hacks.
 * Copyright (C) 2007 Nedko Arnaudov <nedko@arnaudov.name>
 *
 * Machina is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Machina is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Machina.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JACK_COMPAT_H
#define JACK_COMPAT_H


#if defined(JACK_MIDI_NEEDS_NFRAMES)

jack_nframes_t
jack_midi_get_event_count_compat(
  void * port_buffer)
{
#if defined(HAVE_OLD_JACK_MIDI)
  return jack_midi_port_get_info(port_buffer, 0)->event_count;
#else
  return jack_midi_get_event_count(port_buffer, 0);
#endif
}

#define jack_midi_get_event_count jack_midi_get_event_count_compat

int
jack_midi_event_get_compat(
  jack_midi_event_t * event,
  void * port_buffer,
  jack_nframes_t event_index)
{
  return jack_midi_event_get(event, port_buffer, event_index, 0);
}

#define jack_midi_event_get jack_midi_event_get_compat

#else

#if defined(HAVE_OLD_JACK_MIDI)
#error "Old (0.102.20) JACK MIDI API needs nframes (autotools probably gone mad)"
#endif

#endif  /* #if defined(JACK_MIDI_NEEDS_NFRAMES) */

#endif /* JACK_COMPAT_H */
