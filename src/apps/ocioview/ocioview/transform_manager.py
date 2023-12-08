# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from collections import defaultdict
from dataclasses import dataclass
from functools import partial
from typing import Callable, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from .utils import get_glyph_icon


@dataclass
class TransformSubscription:
    """Reference for one item transform subscription."""

    item_model: QtCore.QAbstractItemModel
    item_name: str


class TransformAgent(QtCore.QObject):
    """
    Agent which manages transform fulfillment for a subscription slot.
    """

    item_name_changed = QtCore.Signal(str)
    item_tf_changed = QtCore.Signal(ocio.Transform, ocio.Transform)

    def __init__(self, slot: int, parent: Optional[QtCore.QObject] = None):
        """
        :param slot: Subscription slot number (0-9)
        """
        super().__init__(parent=parent)

        self._slot = slot

    @property
    def slot(self) -> int:
        """
        :return: Subscription slot number
        """
        return self._slot

    def disconnect_all(self) -> None:
        """Disconnect all signals."""
        for signal in (self.item_name_changed, self.item_tf_changed):
            try:
                signal.disconnect()
            except RuntimeError:
                # Signal already disconnected
                pass


class TransformManager:
    """Interface for managing transform subscriptions and subscribers."""

    _tf_subscriptions: dict[int, TransformSubscription] = {}
    _tf_subscribers: dict[int, list[Callable]] = defaultdict(list)
    _tf_menu_subscribers: list[Callable] = []

    @classmethod
    def set_subscription(
        cls, slot: int, item_model: QtCore.QAbstractItemModel, item_name: str
    ) -> None:
        """
        Set the transform for a specific subscription slot, so that
        item transform and name changes are broadcast to all slot
        subscribers.

        :param slot: Subscription slot number between 1-10
        :param item_model: Model for item and its transforms
        :param item_name: Item name
        """
        prev_item_model = None

        # Disconnect previous subscription on target slot
        if slot in cls._tf_subscriptions:
            tf_subscription = cls._tf_subscriptions.pop(slot)
            prev_item_model = tf_subscription.item_model
            tf_agent = tf_subscription.item_model.get_transform_agent(slot)
            tf_agent.disconnect_all()

        # Disconnect other slots with the same item reference
        for other_slot, tf_subscription in list(cls._tf_subscriptions.items()):
            if (
                tf_subscription.item_model == item_model
                and tf_subscription.item_name == item_name
            ):
                tf_agent = tf_subscription.item_model.get_transform_agent(slot)
                tf_agent.disconnect_all()
                del cls._tf_subscriptions[other_slot]

        # Connect new subscription
        tf_subscription = TransformSubscription(item_model, item_name)
        tf_agent = item_model.get_transform_agent(slot)
        tf_agent.item_name_changed.connect(partial(cls._on_item_name_changed, slot))
        tf_agent.item_tf_changed.connect(partial(cls._on_item_tf_changed, slot))
        cls._tf_subscriptions[slot] = tf_subscription

        # Inform menu subscribers of the menu change
        cls._update_menu_items()

        # Inform init subscribers of the new subscription
        for init_callback in cls._tf_subscribers.get(-1, []):
            init_callback(slot)

        # Trigger immediate update to subscribers of this slot
        cls._on_item_tf_changed(slot, *item_model.get_item_transforms(item_name))

        # Repaint views for previous and new model
        if prev_item_model is not None:
            prev_item_model.repaint()
        if prev_item_model is None or prev_item_model != item_model:
            item_model.repaint()

    @classmethod
    def get_subscription_slot(
        cls, item_model: QtCore.QAbstractItemModel, item_name: str
    ) -> int:
        """
        Return the subscription slot number for a transform
        with the provided item model and name, if set.

        :param item_model: Model for item and its transforms
        :param item_name: Item name
        :return: Subscription slot number, or -1 if no subscription is
            set.
        """
        for slot, tf_subscription in cls._tf_subscriptions.items():
            if (
                tf_subscription.item_model == item_model
                and tf_subscription.item_name == item_name
            ):
                return slot
        return -1

    @classmethod
    def get_subscription_slot_color(
        cls, slot: int, saturation: float = 0.5, value: float = 1.0
    ) -> Optional[QtGui.QColor]:
        """
        Return a standard subscription slot color for use in GUI
        elements.

        :param slot: Subscription slot number
        :param saturation: Adjust the color's saturation, which
            defaults to 0.5.
        :param value: Adjust the color's value, which defaults to 1.0
        :return: QColor, if slot number is valid
        """
        if slot != -1:
            return QtGui.QColor.fromHsvF(slot / 10.0, saturation, value)
        else:
            return None

    @classmethod
    def get_subscription_slot_icon(cls, slot: int) -> Optional[QtGui.QIcon]:
        """
        Return a standard subscription slot icon for use in GUI
        elements.

        :param slot: Subscription slot number
        :return: Colorized QIcon if slot number is valid
        """
        if slot != -1:
            slot_word = {
                0: "zero",
                1: "one",
                2: "two",
                3: "three",
                4: "four",
                5: "five",
                6: "six",
                7: "seven",
                8: "eight",
                9: "nine",
            }[slot]
            color = cls.get_subscription_slot_color(slot)
            return get_glyph_icon(f"ph.number-circle-{slot_word}", color=color)
        else:
            return None

    @classmethod
    def get_subscription_menu_items(cls) -> list[tuple[int, str, QtGui.QIcon]]:
        """
        :return: Subscription slots, their names, and respective item
            type icons, for use in subscription menus.
        """
        return [
            (i, s.item_name, cls.get_subscription_slot_icon(i))
            for i, s in sorted(cls._tf_subscriptions.items())
        ]

    @classmethod
    def subscribe_to_transform_menu(cls, menu_callback: Callable) -> None:
        """
        Subscribe to transform menu updates, to be notified when any
        subscription changes.

        :param menu_callback: Menu callback, which will be called with
            a list of tuples with subscription slot, transform name,
            and item icon for each subscription whenever one changes.
        """
        cls._tf_menu_subscribers.append(menu_callback)

        # Trigger immediate menu update to new subscriber
        menu_callback(cls.get_subscription_menu_items())

    @classmethod
    def subscribe_to_transform_subscription_init(cls, init_callback: Callable) -> None:
        """
        Subscribe to transform subscription initialization on all slots.

        :param init_callback: Transform subscription initialization
            callback, which will be called whenever a new transform
            subscription is initialized with the subscription slot
            number.
        """
        cls._tf_subscribers[-1].append(init_callback)

        # Trigger immediate update to init subscriber if a transform subscription
        # exists.
        for slot, tf_subscription in cls._tf_subscriptions.items():
            init_callback(slot)
            break

    @classmethod
    def subscribe_to_transforms_at(cls, slot: int, tf_callback: Callable) -> None:
        """
        Subscribe to transform updates at the given slot number.

        :param slot: Subscription slot number
        :param tf_callback: Transform callback, which will be called
            with the subscription slot number, and forward and inverse
            transforms when either change. All transforms assume a
            scene reference space input.
        """
        tf_subscription = cls._tf_subscriptions.get(slot)
        cls._tf_subscribers[slot].append(tf_callback)

        # Trigger immediate update to new subscriber
        if tf_subscription is not None:
            tf_callback(
                slot,
                *tf_subscription.item_model.get_item_transforms(
                    tf_subscription.item_name
                ),
            )

    @classmethod
    def unsubscribe_from_all_transforms(cls, tf_callback: Callable) -> None:
        """
        Unsubscribe from transform and item name updates at all slot
        numbers.

        :param tf_callback: Previously subscribed transform callback
        """
        for slot, callbacks in cls._tf_subscribers.items():
            if tf_callback in callbacks:
                callbacks.remove(tf_callback)

    @classmethod
    def reset(cls) -> None:
        """
        Drop all transform subscriptions and broadcast empty menus to
        subscribers.
        """
        for slot in reversed(list(cls._tf_subscriptions.keys())):
            tf_subscription = cls._tf_subscriptions.pop(slot)
            tf_agent = tf_subscription.item_model.get_transform_agent(slot)
            tf_agent.disconnect_all()

        # Trigger immediate update to subscribers
        cls._update_menu_items()

    @classmethod
    def _update_menu_items(cls) -> None:
        """Tell all menu subscribers to update their item names."""
        menu_items = cls.get_subscription_menu_items()
        for callback in cls._tf_menu_subscribers:
            callback(menu_items)

    @classmethod
    def _on_item_name_changed(cls, slot: int, item_name: str) -> None:
        """
        Called when a subscription item is renamed, for internal and
        subscriber tracking.
        """
        tf_subscription = cls._tf_subscriptions.get(slot)
        if tf_subscription is not None:
            tf_subscription.item_name = item_name
            cls._on_item_tf_changed(
                slot, *tf_subscription.item_model.get_item_transforms(item_name)
            )
            cls._update_menu_items()

    @classmethod
    def _on_item_tf_changed(
        cls,
        slot: int,
        item_tf_fwd: Optional[ocio.Transform],
        item_tf_inv: Optional[ocio.Transform],
    ) -> None:
        """
        Called when a subscription transform is updated, for internal
        and subscriber tracking.
        """
        tf_subscription = cls._tf_subscriptions.get(slot)
        tf_subscribers = cls._tf_subscribers.get(slot)
        if tf_subscription is not None and tf_subscribers:
            for callback in tf_subscribers:
                callback(slot, item_tf_fwd, item_tf_inv)
