import {mount} from '@vue/test-utils'
import Input from '../../src/components/Input'
import Vue from 'vue';

describe('Input', () => {
    let values = {};

    const form = new Vue({data:{values}});

    const provide = () => {
        return {
            $form: () => ({
                values: form.values
            })
        }
    };

    it('renders correctly', () => {
        const input = mount(Input, {
            propsData: {
                type: "text", name: "test1"
            }
        });
        expect(input.exists()).toBeTruthy();
        expect(input.element.type).toBe("text");
    });

    it('has a default value', () => {

        let input = mount(Input, {
            propsData: {
                type: "text", name: "test1", default: "default"
            },
            provide
        });

        expect(input.vm.value).toBe("default");
    });

    it('updates its value for text', () => {
        const input = mount(Input, {
            propsData: {
                type: "text", name: "test1"
            },
            provide
        });

        const in1 = input.find('input');
        in1.element.value = "Hello world";
        in1.trigger('input');

        expect(input.vm.value).toBe("Hello world");
        expect(values.test1).toBe("Hello world");
    });

    it('updates its value for switch', () => {
        const
            input = mount(Input, {
                propsData: {
                    type: "switch", name: "test3"
                },
                provide
            });

        const in3 = input.find('label');
        in3.trigger('click');

        expect(input.vm.value).toBe(true);
        expect(values.test3).toBe(true);

        in3.trigger('click');

        expect(input.vm.value).toBe(false);
        expect(values.test3).toBe(false);

    });

    it('updates its value for select', () => {
        const input = mount(Input, {
            propsData: {
                type: "select", name: "test2", options: ["1", "2", "3"]
            },
            provide
        });

        const in2 = input.find('select');
        in2.element.value = 1;
        in2.trigger('change');

        expect(input.vm.value).toBe(1);
        expect(values.test2).toBe(1);
    });

});