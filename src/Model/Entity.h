/*
 Copyright (C) 2010-2013 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TrenchBroom__Entity__
#define __TrenchBroom__Entity__

#include "TrenchBroom.h"
#include "SharedPointer.h"
#include "Model/Brush.h"
#include "Model/EntityProperties.h"
#include "Model/ModelTypes.h"
#include "Model/Object.h"
#include "Model/Picker.h"

#include <vector>

namespace TrenchBroom {
    namespace Model {
        class Entity : public Object, public std::tr1::enable_shared_from_this<Entity> {
        public:
            static const Hit::HitType EntityHit;
        private:
            static const String DefaultPropertyValue;
            
            EntityProperties m_properties;
            BrushList m_brushes;
            
            Entity();
        public:
            static EntityPtr newEntity();
            
            BBox3 bounds() const;
            void pick(const Ray3& ray, PickResult& result);
            
            const EntityProperty::List& properties() const;
            bool hasProperty(const PropertyKey& key) const;
            const PropertyValue& property(const PropertyKey& key, const PropertyValue& defaultValue = DefaultPropertyValue) const;
            void addOrUpdateProperty(const PropertyKey& key, const PropertyValue& value);
            
            const PropertyValue& classname(const PropertyValue& defaultClassname = PropertyValues::NoClassname) const;
            
            const BrushList& brushes() const;
            void addBrush(BrushPtr brush);
            void removeBrush(BrushPtr brush);

            template <class Operator, class Filter>
            inline void eachBrush(const Operator& op, const Filter& filter) {
                BrushList::const_iterator it, end;
                for (it = m_brushes.begin(), end = m_brushes.end(); it != end; ++it) {
                    BrushPtr brush = *it;
                    if (filter(brush))
                        op(brush);
                }
            }

            template <class Operator, class Filter>
            inline void eachBrush(Operator& op, const Filter& filter) {
                BrushList::const_iterator it, end;
                for (it = m_brushes.begin(), end = m_brushes.end(); it != end; ++it) {
                    BrushPtr brush = *it;
                    if (filter(brush))
                        op(brush);
                }
            }

            template <class Operator, class Filter>
            inline void eachBrushFace(const Operator& op, const Filter& filter) {
                BrushList::const_iterator it, end;
                for (it = m_brushes.begin(), end = m_brushes.end(); it != end; ++it) {
                    BrushPtr brush = *it;
                    brush->eachBrushFace(op, filter);
                }
            }

            template <class Operator, class Filter>
            inline void eachBrushFace(Operator& op, const Filter& filter) {
                BrushList::const_iterator it, end;
                for (it = m_brushes.begin(), end = m_brushes.end(); it != end; ++it) {
                    BrushPtr brush = *it;
                    brush->eachBrushFace(op, filter);
                }
            }
        private:
            EntityPtr sharedFromThis();
        };
    }
}

#endif /* defined(__TrenchBroom__Entity__) */
